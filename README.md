--- ClickerDecoderNew Notes

Playing around with the BBB and a TSOP38238 IR Receiver Module
I am using an anciet Magnavox VCR controller, so the codes are probably no longer germane.
The Controller prints codes it doesn't know, so you can teach it new tricks.

This version work with new BBB PRU software.
Based on examples from PRUCookbook, including
	05blocks:	neo4.c

(Ins and Outs relative to BBB))
P9_31 = IR input
P9_29 = LED
P9_28 = a toggled test point

