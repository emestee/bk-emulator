#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <libintl.h>
#define _(String) gettext (String)

unsigned char tape_read_val = 1, tape_write_val = 0;
FILE * tape_read_file = NULL;
FILE * tape_write_file = NULL;
unsigned char tape_status = 1; /* 0 = tape moving, 1 = tape stopped */

flag_t fake_tape = 1;	/* Default */

extern int tapeflag;
double tape_read_ticks, tape_write_ticks;
extern double ticks;
void tape_read_start(), tape_read_finish();

static enum { Idle, Addr, Len, Name, Data, Checksum } fake_state = Idle;

#define TAPE_RELAY_DELAY	10000

void tape_init() {
	if (tape_read_file) {
	    if (fake_tape) {
		fclose(tape_read_file);
		fake_state = Idle;
	    } else {
		pclose(tape_read_file);
	    }
	    tape_read_file = NULL;
	}

	if (fake_tape) {
	    if (tape_write_file) {
		fclose(tape_write_file);
		tape_write_file = 0;
	    }
	} else if (tape_write_file == NULL) {
		tape_write_file = popen("readtape", "w");
		if (tape_write_file) {
			fprintf(stderr, _("readtape open successful\n"));
		} else perror("readtape");
	}
}


void tape_write(unsigned status, unsigned val) {
    if (fake_tape) {
	if (status != tape_status) {
		ticks += TAPE_RELAY_DELAY;
		tape_status = status;
	}
	if (tape_read_file && tape_status) {
		fclose(tape_read_file);
		tape_read_file = 0;
		fake_state = Idle;
	}
	return;
    }
    if (status != tape_status) {
	ticks += TAPE_RELAY_DELAY;
	tape_status = status;
	fprintf(stderr, _("Tape %s\n"), tape_status ? _("OFF") : _("ON"));
	if (!tape_status) {
	    tape_read_ticks = tape_write_ticks = ticks;
	    tape_read_start();
	} else {
	    fflush(tape_write_file);
	    if(tape_read_file) tape_read_finish();
	}
    }
    if (!tape_status && val != tape_write_val) {
    	unsigned delta = ticks - tape_write_ticks;
    	while (delta) {
		unsigned delta2 = (delta <= 32767) ? delta : 32767;

		fputc((delta2 >> 8) | (tape_write_val << 7), tape_write_file);
		fputc(delta2 & 0xFF, tape_write_file);
		delta -= delta2;
    	}
    	tape_write_ticks = ticks;
	tape_write_val = val;	
    }
}

int
tape_read() {
    unsigned delta;
    if (fake_tape) {
	/* Random noise */
	tape_read_val = ((unsigned) (ticks / 1001.0)) & 1;
	return tape_read_val;
    }
    if (tape_status || tape_read_file == NULL) {
	/* some programs want to see tape read bit changing if
	 * the tape motor is OFF. Why - I don't know.
	 */
	return tape_read_val = !tape_read_val;
    }
    while (tape_read_file && ticks > tape_read_ticks) {
	int c1 = fgetc(tape_read_file);
	int c2 = fgetc(tape_read_file);
	if (c2 == EOF) {
		fprintf(stderr, _("End of tape\n"));
		pclose(tape_read_file);
		tape_read_file = 0;
	}
	delta = c1 << 8;
	delta |= c2;
	tape_read_val = delta >> 15;
	delta &= 0x7FFF;
	tape_read_ticks += delta;
    }
    return tape_read_val;
}

static char bk_filename[17];
static char unix_filename[17];

/* 
 * Returns the raw 16-byte file name in bk_filename,
 * and the name with trimmed trailing spaces in unix_filename.
 */
void
get_emt36_filename() {
	int i;
	d_word base;
	lc_word(0306, &base);
	for (i = 0; i < 8; i++) {
		d_word d;
		lc_word(base + 6 + 2*i, &d);
		bk_filename[2*i] = d & 0xff;
		bk_filename[2*i+1] = d >> 8;
	}
	bk_filename[16] = '\0';
	for (i = 15; i >= 0 && bk_filename[i] == ' '; unix_filename[i--]='\0');
	for (; i >= 0; i--) unix_filename[i] = bk_filename[i];
}

/*
 * When download is requested, attempt to find a file with
 * name specified in the tape drive control block and to
 * open it using maketape.
 */
void
tape_read_start() {
	static char buf[80];
	if (tapeflag) {
	/* attempts to manipulate the tape drive relay will be ignored */
		return;
	}
	get_emt36_filename();	
	sprintf(buf, "maketape '%s' '%s'", bk_filename, unix_filename);
#ifdef VERBOSE_TAPE
	fprintf(stderr, _("Calling (%s)\n"), buf);
#endif
	tape_read_file = popen(buf, "r");
	if (tape_read_file) {
		tape_read_ticks = ticks;
	} else perror(unix_filename);
}

void
tape_read_finish() {
	if (!tape_read_file) return;
	pclose(tape_read_file);
	tape_read_file = 0;
#ifdef VERBOSE_TAPE
	fprintf(stderr, _("Closed maketape\n"));
#endif
}

/* Sets word 0314 to the strobe length (say, 10) */
void fake_tuneup_sequence() {
	sc_word(0314, 10);
        /* Fake RTS PC */
        pop( &pdp, &pdp.regs[PC] );
}

/* Nothing to tune up, skip to reading without tune-up */
void fake_array_with_tuneup() {
	pdp.regs[PC] = 0117336;
}

/* To return an error on a non-existent file, we return addr 0,
 * length 1, and the actual contents of byte @ 0, but the checksum
 * is deliberately incorrect.
 */
void fake_read_strobe() {
	static int bitnum;
	static unsigned checksum;
	static unsigned char curbyte;
	static unsigned curlen;
	static unsigned curaddr;
	int bit;
	if (fake_state == Idle && !tape_read_file) {
		/* First time here, find which file to open */
		get_emt36_filename();
		tape_read_file = fopen(unix_filename, "r");
		fprintf(stderr, _("Will read unix file <%s> under BK name <%s>\n"),
			unix_filename, bk_filename);
		fake_state = Addr;
		bitnum = 0;
		checksum = 0;
		curlen = curaddr = 0;
	}
	switch (fake_state) {
	case Idle:
		/* We're not supposed to get here */
		fprintf(stderr, _("Asked for strobe in Idle state?\n"));
		fclose(tape_read_file);
		bit = 0;
		break;
	case Addr:
		if (!(bitnum & 7)) {
			curbyte = tape_read_file ? fgetc(tape_read_file) : 0;
			curaddr |= curbyte << bitnum;
		}
		bit = (curbyte >> ((bitnum++) & 7)) & 1;
		if (bitnum == 16) {
			fprintf(stderr, _("File address will be %o\n"), curaddr);
			fake_state = Len;
			bitnum = 0;
		}
                break;
	case Len:
		if (!(bitnum & 7)) {
                        curbyte = tape_read_file ? fgetc(tape_read_file) :
				bitnum ? 0 : 1;
			curlen |= curbyte << bitnum;
                }
		bit = (curbyte >> ((bitnum++) & 7)) & 1;
		if (bitnum == 16) {
			fprintf(stderr, _("File length will be %o\n"), curlen);
                        fake_state = Name;
                        bitnum = 0;
                }
                break;

	case Name:
		bit = (bk_filename[bitnum >> 3] >> (bitnum & 7)) & 1;
		bitnum++;
		if (bitnum == 16 * 8) {
			fake_state = Data;
			bitnum = 0;	
		}
		break;
	case Data:
		if (!(bitnum & 7)) {
			if (tape_read_file) {
				curbyte = fgetc(tape_read_file);
			} else {
				d_word w;
				lc_word(0, &w);
				curbyte = w & 0xff;
			}
			checksum += curbyte;
			if (checksum & 0200000) {
				checksum = (checksum & 0xFFFF) + 1;
			}
		}
		bit = (curbyte >> ((bitnum++) & 7)) & 1;
		if (bitnum == curlen * 8) {
			if (!tape_read_file) checksum++;
			fprintf(stderr, _("Checksum will be %06o\n"), checksum);
			fake_state = Checksum;
			bitnum = 0;
		}
		break;
	case Checksum:
		bit = (checksum >> bitnum++) & 1;
		if (bitnum == 16) {
			if (tape_read_file) fclose(tape_read_file);
			tape_read_file = 0;
			fake_state = Idle;
		}
		break;
	}
        pdp.regs[4] = bit ? 15 : 5;
        /* Fake RTS PC */
        pop( &pdp, &pdp.regs[PC] );
}

void
fake_write_file() {
	c_addr base;
	lc_word(0306, &base);
	get_emt36_filename();
	tape_write_file = fopen(unix_filename, "w");
	fprintf(stderr, "Will%swrite BK file <%s> under unix file name <%s>\n",
		tape_write_file ? " " : " NOT ", bk_filename, unix_filename);
	if (tape_write_file) {
		d_word addr;
	        d_word length;
		lc_word(base + 2, &addr);
		fputc(addr & 0xff, tape_write_file);
		fputc(addr >> 8, tape_write_file);
		lc_word(base + 4, &length);
		fputc(length & 0xff, tape_write_file);
		fputc(length >> 8, tape_write_file);
		for (; length; addr++, length--) {
			d_word w;
			lc_word(addr, &w);
			if (addr & 1) w >>= 8;
			fputc(w & 0xff, tape_write_file);
		}
		fclose(tape_write_file);
		tape_write_file = 0;
		sl_byte(&pdp, base+1, 0);
	} else {
		perror(unix_filename);
		sl_byte(&pdp, base+1, 1);
	}
	/* Fake RTS PC */
	pop( &pdp, &pdp.regs[PC] );
}

