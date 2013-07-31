#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <libintl.h>
#define _(String) gettext (String)

unsigned char covox_val;
unsigned int covox_age;

covox_init() {
	covox_val = 0;
	covox_age = ~0;
}

covox_read(c_addr addr, d_word *word)
{
	*word = 0;	/* pulldown */
	return OK;
}

covox_write(c_addr addr, d_word word)
{
	covox_val = word & 0xFF;
	covox_age = 0;
	return OK;
}

covox_bwrite(c_addr addr, d_byte byte) {
	d_word offset = addr & 1;
	d_word word;
	if (offset == 0) {
		covox_val = byte;
	} else {
		covox_val = 0;
	}
	covox_age = 0;
	return OK;
}

