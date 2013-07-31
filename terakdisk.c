#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libintl.h>
#define _(String) gettext (String)

/* Terak floppy images are 128*26*76 = 252928 bytes, single-sided */
#define SECSIZE 64 /* words */
#define SECPERTRACK 26
#define MAXTRACK 76
typedef enum {
	nopD, rtcD, stepinD, stepoutD, readtsD, readD, writeD, delD
} disk_cmd;

typedef enum {
	enF = 1, headF = 020, intrF = 0100, doneF = 0200,
	track0F = 01000, delF = 02000, wrprF = 04000, crcF = 010000,
	lateF = 020000, syncF = 040000, errF = 0100000
} disk_flg;

/* Why bother, let's memory-map the files! */
typedef struct {
	unsigned length;
	unsigned short * image;
	unsigned short * ptr;
	unsigned char track;
	disk_cmd cmd;
	unsigned char ro;
	unsigned char motor;
	unsigned char inprogress;
	unsigned char crc;
	unsigned char cursec;
} tdisk_t;

tdisk_t tdisks[4];
static int selected = -1;

void tdisk_open(tdisk_t * pdt, char * name) {
	int fd = open(name, O_RDWR);
	if (fd == -1) {
		pdt->ro = 1;
		fd = open(name, O_RDONLY);
	}
	if (fd == -1) {
		perror(name);
		return;
	}
	pdt->length = lseek(fd, 0, SEEK_END);
	if (pdt->length % SECSIZE) {
		fprintf(stderr, _("%s is not an integer number of blocks\n"), name);
		close(fd);
		return;
	}
	pdt->image = mmap(0, pdt->length, PROT_READ | (pdt->ro ? 0 : PROT_WRITE), MAP_SHARED, fd, 0);
	if (pdt->image == MAP_FAILED) {
		pdt->image = 0;
		perror(name);
	}
	if (pdt->ro) {
		fprintf(stderr, _("%s will be read only\n"), name);
	}
}

/* Are there any interrupts to open or close ? */

int tdisk_init() {
	static char init_done = 0;
	int i;
	if (!init_done) {
		disk_open(&tdisks[0], floppyA);	
		disk_open(&tdisks[1], floppyB);	
		disk_open(&tdisks[2], floppyC);	
		disk_open(&tdisks[3], floppyD);	
		init_done = 1;
	}
	for (i = 0; i < 4; i++) {
		tdisks[i].ptr = NULL;
		tdisks[i].track =
		tdisks[i].motor =
		tdisks[i].inprogress = 0;
	}
	selected = -1;
	return OK;
}

void tdisk_finish() {
	int i;
	for (i = 0; i < 4; i++) {
		if (!tdisks[i].image)
			continue;
		munmap(tdisks[i].image, tdisks[i].length);
	}	
}


static inline unsigned unit(d_word word) {
	return (word >> 8) & 3;
}

static inline disk_cmd cmd(d_word word) {
	return (word >> 1) & 7;
}
int
tdisk_read(c_addr addr, d_word *word) {
	d_word offset = addr - TERAK_DISK_REG;
	tdisk_t * pdt = &tdisks[selected];
	int index;
	switch(offset) {
	case 0: /* status */
		if (selected == -1) {
		*word = errF | doneF;
		break;
		}
		*word = (pdt->track ? 0 : track0F) | (pdt->ro ? wrprF : 0) | headF | doneF;
		if (!pdt->inprogress) {
		/* no operation started yet */
			return OK;
		} else switch (pdt->cmd) {
			case nopD:
			case rtcD:
			case writeD:
			case delD:
				break;
			case stepinD:
				if (pdt->track == MAXTRACK)
					*word |= errF;
				else if (pdt->inprogress) {
					pdt->track++;
					fprintf(stderr, "trk = %d\n", pdt->track);
				}
				break;
			case stepoutD:
				if (pdt->track == 0)
					*word |= errF;
				else if (pdt->inprogress) {
					pdt->track--;
					fprintf(stderr, "trk = %d\n", pdt->track);
				}
				break;
			case readtsD:
				/* present all sectors round-robin */
				if (pdt->inprogress) {
					pdt->cursec %= SECPERTRACK;
					pdt->cursec++;				
				}
				break;
			case readD:
				fprintf(stderr, "Reading track %d, sector %d\n",
					pdt->track, pdt->cursec);
				pdt->ptr = pdt->image +
					pdt->track*SECPERTRACK*SECSIZE +
					(pdt->cursec-1)*SECSIZE;
				break;
		}
		pdt->inprogress = 0;
		break;
	case 2: /* data */
		switch (pdt->cmd) {
		case readtsD:
			*word = pdt->cursec << 8 | pdt->track;
			fprintf(stderr, "Trk/sec = %d/%d\n", pdt->track, pdt->cursec);
			break;
		case readD:
			*word = *pdt->ptr++;
			break;
		default:
			*word = 0;
		}
	break;
	}
	return OK;
}

int
tdisk_write(c_addr addr, d_word word) {
	d_word offset = addr - TERAK_DISK_REG;
	tdisk_t * pdt;
	switch (offset) {
	case 0:         /* control port */
		if (word) {
			/* Print a message if something other than turning
			 * all drives off is requested
			 */
			// fprintf(stderr, _("Writing disk cmd, data %06o\n"), word);
		}
		selected = unit(word);
		if (selected >= 0) {
			pdt = &tdisks[selected];
			if (pdt->inprogress)
				return BUS_ERROR;
			pdt->inprogress = word & enF;
			pdt->cmd = cmd(word);
			if (pdt->inprogress && word & intrF) switch (pdt->cmd) {
				case nopD:
					ev_register(TTY_PRI, service, TICK_RATE*100/25, 0250);
					break;
				case rtcD:
					ev_register(TTY_PRI, service, TICK_RATE/50, 0250);
					break;
				default:
					fprintf(stderr, "Interrupt requested\n");
					ev_register(TTY_PRI, service, TICK_RATE/1000, 0250);
			}
		}
		break;
	case 2:	/* data port */
		fprintf(stderr, _("Writing disk data reg, data %06o\n"), word);
		break;
	}
	return OK;
}

int
tdisk_bwrite(c_addr addr, d_byte byte) {
	return disk_write(addr & ~1, byte);
}
