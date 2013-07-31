#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <libintl.h>
#define _(String) gettext (String)

FILE * io_printer_file = NULL;
#define STROBE 0400

printer_init() {
	if (NULL == io_printer_file && printer_file) {
	    io_printer_file = fopen(printer_file, "w");
	    if (NULL == io_printer_file) {
		perror(printer_file);
	    }
	}

	return OK;
}

printer_read(addr, word)
c_addr addr;
d_word *word;
{
	static d_word ready = 0;
	if (io_printer_file) {
		*word = ready ^= STROBE;   /* flip-flop */
	} else {
		*word = 0;	/* pulldown */
	}
	return OK;
}

printer_write(addr, word)
c_addr addr;
d_word word;
{
	/* To be exact, posedge strobe must be checked,
	 * but there is no use writing a new byte without
	 * bringing the strobe down first; so we do it level-based.
	 */
	if (io_printer_file && (word & STROBE)) {
		fputc(word & 0xFF, io_printer_file);
	}
	return OK;
}

printer_bwrite(c_addr addr, d_byte byte) {
	d_word offset = addr & 1;
	d_word word;
	printer_read(addr & ~1, &word);
	if (offset == 0) {
		word = (word & 0177400) | byte;
	} else {
		word = (byte << 8) | (word & 0377);
	}
	return printer_write(addr & ~1, word);
}

