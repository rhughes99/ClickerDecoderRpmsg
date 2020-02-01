// Use rpmsg to communicate via /dev/rpmsg_pru30
#include <stdint.h>
#include <stdio.h>
//#include <stdlib.h>			// atoi
//#include <string.h>
#include <pru_cfg.h>
#include <pru_intc.h>
#include <rsc_types.h>
#include <pru_rpmsg.h>
#include "resource_table_0.h"

volatile register uint32_t __R30;	// Output
volatile register uint32_t __R31;	// Input

/* Host-0 Interrupt sets bit 30 in register R31 */
#define HOST_INT		((uint32_t) 1 << 30)

/* The PRU-ICSS system events used for RPMsg are defined in the Linux device tree
 * PRU0 uses system event 16 (To ARM) and 17 (From ARM)
 * PRU1 uses system event 18 (To ARM) and 19 (From ARM)
 */
#define TO_ARM_HOST		16
#define FROM_ARM_HOST	17

/*
* Using name 'rpmsg-pru' will probe rpmsg_pru driver found
* at linux-x.y.z/drivers/rpmsg/rpmsg_pru.c
*/
#define CHAN_NAME		"rpmsg-pru"
#define CHAN_DESC		"Channel 30"
#define CHAN_PORT		30

/*
 * Used to make sure Linux drivers are ready for RPMsg communication
 * Found at linux-x.y.z/include/uapi/linux/virtio_config.h
 */
#define VIRTIO_CONFIG_S_DRIVER_OK	4

unsigned char payload[RPMSG_BUF_SIZE];

//____________________
void main(void)
{
	struct pru_rpmsg_transport transport;
	uint16_t src, dst, len;
	volatile uint8_t *status;

	int i, code, sample;
	uint32_t LED, IR_IN, TEST;

	// Set I/O constants
	LED   = 0x1<<1;		// P9_29, output
	IR_IN = 0x1<<0;		// P9_31, input
	TEST  = 0x1<<3;		// P9_28, output

	// Turn LED off & TEST=0
//	__R30 &= ~LED;
//	__R30 &= ~TEST;
	__R30 &= ~(LED | TEST);

	// Allow OCP master port access by PRU so PRU can read external memories
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

	// Clear status of PRU-ICSS system event that ARM will use to 'kick' us
	CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;

	// Make sure Linux drivers are ready for RPMsg communication
	status = &resourceTable.rpmsg_vdev.status;
	while (!(*status & VIRTIO_CONFIG_S_DRIVER_OK));

	// Initialize RPMsg transport structure
	pru_rpmsg_init(&transport, &resourceTable.rpmsg_vring0, &resourceTable.rpmsg_vring1, TO_ARM_HOST, FROM_ARM_HOST);

	// Create RPMsg channel between PRU and ARM user space using transport structure
	while (pru_rpmsg_channel(RPMSG_NS_CREATE, &transport, CHAN_NAME, CHAN_DESC, CHAN_PORT) != PRU_RPMSG_SUCCESS);

	while (1)
	{
		// Check bit 30 of register R31 to see if ARM has kicked us
		if (__R31 & HOST_INT)
		{
			// Clear event status
			CT_INTC.SICR_bit.STS_CLR_IDX = FROM_ARM_HOST;

			// Receive all available messages, multiple messages can be sent per kick
			// We start waiting for next clicker input after receiving message
			// Contents of message are not used
			while (pru_rpmsg_receive(&transport, &src, &dst, payload, &len) == PRU_RPMSG_SUCCESS)
			{
				// Input format is irrelevant; data not used
//				index = atoi(payload);			// index = 1st int in payload

				// Wait for 0 input from clicker; quiescent output of sensor = 1
				while ((__R31 & IR_IN) == IR_IN);

				__R30 |= LED;		// LED on, start sampling

				// Start sampling, delay 450 usec to sample pulse centers
				__delay_cycles(90000);	// 450 us

				// Looking for 0 1 0 preamble
//				__R30 |= TEST;		// TEST=1
				if ((__R31 & IR_IN) == 0)
				{
					__delay_cycles(180000);		// 900 us
					__R30 &= ~TEST;		// TEST=0
					if ((__R31 & IR_IN) == 1)
					{
						__delay_cycles(180000);
//						__R30 |= TEST;		// TEST=1
						if ((__R31 & IR_IN) == 0)
						{
							// Now ready for 25 bits of clicker code
							code = 0;
							for (i=0; i<25; i++)
							{
								__delay_cycles(90000);
								__R30 &= ~TEST;		// TEST=0
								__delay_cycles(90000);
								__R30 |= TEST;		// TEST=1, so rising edge of TEST is sample time

								sample = __R31 & IR_IN;
								code = (code<<1) | sample;		// shift code left & insert sample as lsb

//								sample = (__R31 & IR_IN)<<i;	// bit 0 so no shifting of input necessary
//								code |=	sample;					//  but shift left based on sample number
							}

							// Complete code received, put in payload & send to Controller
							// Not thrilled with order, but it matches original verison of ClickerDecoder
							payload[0] = 255;					// indicates code received
							payload[1] =  code & 0x000000FF;
							payload[2] = (code & 0x0000FF00) >> 8;
							payload[3] = (code & 0x00FF0000) >> 16;
							payload[4] = (code & 0xFF000000) >> 24;

							pru_rpmsg_send(&transport, dst, src, payload, 5);

							__R30 &= ~LED;		// LED off
						}
					}
				}
			}
		}
	}
}
