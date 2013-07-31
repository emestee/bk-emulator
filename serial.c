#include "defines.h"

#define READY_IN	0200
#define READY_OUT	040
#define DATA_BIT	020
#define PROBE		0xBC

unsigned char retbyte;
unsigned char * data_ptr = 0;
char bitnum = 8;
unsigned char ready_out;
FILE * ser_in = 0;

/* Returns the ready bit and the data bit in the appropriate positions;
 * the emulator is always ready to accept data. The data bits are only
 * returned when ready_out is true.
 */
d_word
serial_read() {
	unsigned char ret = READY_IN;
	if (!data_ptr || !ready_out) {
		/* Idle state - continuous stop bits */
		return READY_IN | DATA_BIT;
	}
	if (bitnum == -1) {
		bitnum = 0;
		/* start bit is 0 */
		return READY_IN & ~DATA_BIT;
	}
	if (bitnum == 8) {
		/* Byte ended - stop bit is 1 */
		return READY_IN | DATA_BIT;
	}
	ret |= *data_ptr & (1<<bitnum++) ? DATA_BIT : 0;
	if (!ser_in) {
		ser_in = fopen("serial.in", "r");
	}
	if (bitnum == 8) {
		int c = getc(ser_in);
		if (c == EOF) data_ptr = 0;
		retbyte = c;
		bitnum = -1;
	}
	return ret;
}

unsigned char ser_out = 1;
unsigned char forming;
char curbit = -1;

/* This is a cheat: we know that for each data bit there is a write
 * to the port: no need to compare time marks.
 */
void
serial_write(d_word w) {
	unsigned char cur = (w & DATA_BIT) != 0;

	if (ready_out != (w & READY_OUT)) {
		ready_out = w & READY_OUT;
	}

	if (!ready_out) {
		/* Idle state */
		bitnum = 8;
	} else {
	    if (!data_ptr) {
		/* Set up the probe byte */
		retbyte = PROBE;
		data_ptr = &retbyte;
	    }
	    bitnum = -1;
	}
	if (curbit >= 0) {
		forming |= cur << curbit++;
		if (curbit == 8) {
			curbit = -1;
			if (forming != PROBE) {
				data_ptr = 0;
				fprintf(stderr, "Serial line probe failed\n");
			}
		}
	} else if (ser_out && !cur) {
		/* Next write will be data bit */
		curbit = 0;
	}
	ser_out = cur;
}
