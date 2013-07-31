#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
/* #include <linux/soundcard.h> */
#include <libintl.h>
#define _(String) gettext (String)

extern double io_sound_count;
extern int io_sound_age;
extern flag_t nflag, fullspeed;
unsigned io_sound_val = 0;
flag_t io_stop_happened = 0;
flag_t telegraph_enabled = 0; 	/* Default */

io_init() {
	sound_init();
	tape_init();
	return OK;
}

io_read(addr, word)
c_addr addr;
d_word *word;
{
	int tape_bit;
	tape_bit = tape_read() << 5;

	/* the high byte is 0200 for BK-0010
	 * and 0300 for BK-0011
	 */
	*word = 0100000 | (bkmodel << 14) |
		(telegraph_enabled ? serial_read() : 0200) |
		key_pressed |
		tape_bit |
		io_stop_happened;
	io_stop_happened = 0;
	return OK;
}

/* Include tape drive relay into sound as well */
io_write(addr, word)
c_addr addr;
d_word word;
{
	d_word offset = addr - IO_REG;
	unsigned oldval = io_sound_val;
	if (bkmodel && word & 04000) {
		pagereg_write(word);
		return OK;
	}
	io_sound_val = word & 0300;
	if (io_sound_val != oldval) {
		if (fullspeed) io_sound_count = ticks;
		    io_sound_age = 0;
	}
	/* status, value */
	tape_write((word >> 7) & 1, (word >> 6) & 1);
	if (telegraph_enabled) {
		serial_write(word);
	}
	return OK;
}

io_bwrite(c_addr addr, d_byte byte) {
	d_word offset = addr - IO_REG;
	unsigned oldval = io_sound_val;
	if (offset == 0) {
	    io_sound_val = byte & 0300;
	    if (io_sound_val != oldval) {
		    if (fullspeed) io_sound_count = ticks;
		    io_sound_age = 0;
	    }
	    tape_write((byte >> 7) & 1, (byte >> 6) & 1);
	    if (telegraph_enabled) {
		    serial_write(byte);
	    }
	} else if (bkmodel && byte & 010) {
		pagereg_bwrite(byte);
	}
	return OK;
}

#define LINE_RST 0176560
#define LINE_RDT 0176562
#define LINE_WST 0176564
#define LINE_WDT 0176566

FILE * irpslog = 0;
enum { IdleL, NameL, HeaderL, BodyL, TailL } lstate = 0;
unsigned char rdbuf = 0;

line_init() {
	irpslog = fopen("irps.log", "w");
}

line_read(addr, word)
c_addr addr;
d_word *word;
{
	switch (addr) {
	// Always ready
	case LINE_RST: case LINE_WST:
		*word = 0200;
		break;
	case LINE_WDT:
		*word = 0;
		break;
	case LINE_RDT:
		*word = rdbuf;
		// rdbuf = next byte
		break;
	}
	return OK;
}

line_write(addr, word)
c_addr addr;
d_word word;
{
	switch (addr) {
	case LINE_WDT:
		return line_bwrite(addr, word);
	case LINE_RDT:
		// no effect
		break;
	case LINE_RST: case LINE_WST:
		// no effect yet
		break;
	}
	return OK;
}

int subcnt;
unsigned char fname[11];
unsigned short file_addr, file_len;
line_bwrite(addr, byte)
c_addr addr;
d_byte byte;
{
	fputc(byte, irpslog);
	switch (lstate) {
	case IdleL:
		switch (byte) {
		case 0: // stop
			fprintf(stderr, "Stop requested\n");
			break;
		case 1: // start
			fprintf(stderr, "Start requested\n");
			rdbuf = 1;
			break;
		case 2: // write
			fprintf(stderr, "File write requested\n");
			rdbuf = 2;
			lstate = NameL;
			subcnt = 0;
			break;
		case 3: // read
			fprintf(stderr, "File read requested\n");
			rdbuf = 3;
			break;
		case 4: // fake read
			fprintf(stderr, "Fake read requested\n");
			rdbuf = 4;
			break;
		default:
			fprintf(stderr, "Unknown op %#o\n", byte);
			rdbuf = 0377;
			break;
		}
		break;
	case NameL:
		fname[subcnt++] = byte;
		rdbuf = 0;
		if (subcnt == 10) {
			fprintf(stderr, " file name %s\n", fname);
			lstate = HeaderL;
			subcnt = 0;
		}
		break;
	case HeaderL:
		fprintf(stderr, "Got %#o\n", byte);
		switch (subcnt) {
		case 0:
			file_addr = byte;
			break;
		case 1:
			file_addr |= byte << 8;
			break;
		case 2:
			file_len = byte;
			break;
		case 3:
			file_len |= byte << 8;
			break;
		}
		if (++subcnt == 4) {
			fprintf(stderr, " file addr %#o, len %#o\n", file_addr, file_len);
			lstate = BodyL;
			subcnt = 0;
		}
		break;
	case BodyL:
		if (++subcnt == file_len) {
			lstate = IdleL;
			subcnt = 0;
			fprintf(stderr, "Finished\n");
		}
	}
	return OK;
}
