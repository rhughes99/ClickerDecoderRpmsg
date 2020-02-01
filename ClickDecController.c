/*	Clicker Decoder Controller
	Tuned for Magnavox VCR remote: VCR & TV
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void myShutdown(int sig);

unsigned char running;

//____________________
int main (void)
{
	unsigned int theCmd, lastCmd;
	unsigned char index, readBuffer[8];
//	unsigned char writeBuffer[] = {"1 2 3 4\n"};

	FILE *fp;
	fp = fopen("/dev/rpmsg_pru30", "w+");
	if (fp == NULL)
	{
		printf("*** ERROR opening /dev/rpmsg_pru30\n");
		exit(EXIT_FAILURE);
	}
//	fprintf(fp, "%d %d %d %d\n", 1, 2, 3, 4);	// data irrelevant?

	(void) signal(SIGINT, myShutdown);

	running = 1;
	lastCmd = 0;
	index = 1;
	fprintf(fp, "%d %d %d\n", 1, 2, 3);		// kicks PRU, data irrelevant
	printf("\nClickerDecoder running...\n");
	do
	{
//		usleep(5000);		// 1 command takes ~24 ms
		sleep(0.1);

		fread(readBuffer, 1, 5, fp);
		if (readBuffer[0] == 255)
		{
			theCmd = readBuffer[1] | readBuffer[2]<<8 | readBuffer[3]<<16 | readBuffer[4]<<24;
//			printf("new theCmd =  %d\n", theCmd);
			if (theCmd != lastCmd)
			{
				switch (theCmd)
				{
					case 19712843:								// 0x12CCB4B
					case 11324235:								// 0x0ACCB4B
						printf("--- VCR POWER ---\n");
						break;

					case 11184971:
					case 19573579:
						printf("--- TV POWER ---\n");
						break;

					case 11326803:
					case 19715411:
						printf("VCR/TV\n");
						break;

					case 11326285:
					case 19714893:
						printf("VCR: EJECT\n");
						break;

					case 11326669:
					case 19715277:
						printf("VCR: PLAY\n");
						break;

					case 11326643:
					case 19715251:
						printf("VCR: REW\n");
						break;

					// And a while lot more...

					default:
						printf("Unknown cmd: %d (0x%X)\n", theCmd, theCmd);
				}
				lastCmd = theCmd;
			}
			fprintf(fp, "%d %d %d\n", 1, 2, 3);		// clear  payload & re-kick PRU
		}
	} while (running);

	printf ("---Shutting down...\n");
	fclose(fp);
	return EXIT_SUCCESS;
}

//____________________
void myShutdown(int sig)
{
	// ctrl-c
	running = 0;
	(void) signal(SIGINT, SIG_DFL);		// reset signal handling of SIGINT
}
