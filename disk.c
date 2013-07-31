#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libintl.h>
#define _(String) gettext (String)

/* let's operate on 819200 byte images first */
#define SECSIZE 512
#define SECPERTRACK 10

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Why bother, let's memory-map the files! */
typedef struct {
	unsigned length;
	unsigned short * image;
	unsigned short * ptr;
	unsigned char track;
	unsigned char side;
	unsigned char ro;
	unsigned char motor;
	unsigned char inprogress;
	unsigned char crc;
	unsigned char need_sidetrk;
	unsigned char need_sectsize;
	unsigned char cursec;
} disk_t;

disk_t disks[4];
static int selected = -1;

void do_disk_io(int drive, int blkno, int nwords, int ioaddr);

void disk_open(disk_t * pdt, char * name) {
	int fd = open(name, O_RDWR|O_BINARY);
	if (fd == -1) {
		pdt->ro = 1;
		fd = open(name, O_RDONLY|O_BINARY);
	}
	if (fd == -1) {
		perror(name);
		return;
	}
	pdt->length = lseek(fd, 0, SEEK_END);
	if (pdt->length == -1) perror("seek");
	if (pdt->length % SECSIZE) {
		fprintf(stderr, _("%s is not an integer number of blocks: %d bytes\n"), name, pdt->length);
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

int disk_init() {
	static char init_done = 0;
	int i;
	if (!init_done) {
		disk_open(&disks[0], floppyA);	
		disk_open(&disks[1], floppyB);	
		disk_open(&disks[2], floppyC);	
		disk_open(&disks[3], floppyD);	
		init_done = 1;
	}
	for (i = 0; i < 4; i++) {
		disks[i].ptr = NULL;
		disks[i].track = disks[i].side =
		disks[i].motor = disks[i].inprogress = 0;
	}
	selected = -1;
	return OK;
}

void disk_finish() {
	int i;
	for (i = 0; i < 4; i++) {
		if (!disks[i].image)
			continue;
		munmap(disks[i].image, disks[i].length);
	}	
}

/* The index hole appears for 1 ms every 100 ms,
 */
int index_flag() {
	extern double ticks;
	unsigned msec = ticks / (TICK_RATE/1000);
	return (msec % 100 == 0);
}

#define FILLER 047116
#define LAST 0120641
#define IDXFLAG 0120776
#define DATAFLAG 0120773
#define ENDFLAG 0120770

unsigned short index_marker[] = {
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER,  
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, 
0, 0, 0, 0, 0, 0, LAST, IDXFLAG
};

#define IDXMRKLEN (sizeof(index_marker)/sizeof(*index_marker))

unsigned short data_marker[] = {
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER,
FILLER, FILLER, FILLER, 0, 0, 0, 0, 0, 0, LAST, DATAFLAG
};
#define DATAMRKLEN (sizeof(data_marker)/sizeof(*data_marker))

unsigned short end_marker[] = {
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER,
FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER, FILLER,
FILLER, FILLER, 0, 0, 0, 0, 0, 0, LAST, ENDFLAG
};
#define ENDMARKLEN (sizeof(end_marker)/sizeof(*end_marker))

unsigned short
disk_read_word(disk_t * pdt) {
	unsigned short ret;
	if (pdt->need_sidetrk) {
		ret = (pdt->side << 8) | pdt->track;
		pdt->need_sidetrk = 0;
		pdt->need_sectsize = 1;
	} else if (pdt->need_sectsize) {
		ret = ((pdt->cursec+1) << 8) | 2;
		pdt->need_sectsize = 0;
		pdt->ptr = data_marker;
	} else {
		ret = *pdt->ptr++;
		if (pdt->ptr == index_marker + IDXMRKLEN) {
			pdt->need_sidetrk = 1;
		} else if (pdt->ptr == data_marker + DATAMRKLEN) {
			pdt->ptr = pdt->image +
			(((pdt->track * 2 + pdt->side) * SECPERTRACK) + pdt->cursec) * SECSIZE/2;
		} else if (pdt->ptr ==
			pdt->image + (((pdt->track * 2 + pdt->side) * SECPERTRACK) + pdt->cursec+1) * SECSIZE/2) {
			pdt->ptr = end_marker;
		} else if (pdt->ptr == end_marker + ENDMARKLEN) {
			if (++pdt->cursec == SECPERTRACK) pdt->inprogress = 0;
			pdt->ptr = index_marker;
		}
	}
	return ret;
}

int
disk_read(c_addr addr, d_word *word) {
	d_word offset = addr - DISK_REG;
	disk_t * pdt = &disks[selected];
	int index;
        switch(offset) {
        case 0: /* 177130 */
	if (selected == -1) {
		fprintf(stderr, _("Reading 177130, returned 0\n"));
		*word = 0;
		break;
	}
	index = index_flag();
	if (index) {
		pdt->ptr = index_marker;
		pdt->cursec = 0;
		pdt->inprogress = pdt->image != 0;
	}
	*word = (pdt->track == 0) |
		((pdt->image != 0) << 1) |
		(pdt->ro << 2) |
		(pdt->inprogress << 7) |
		(pdt->crc << 14) |
		(index << 15);
#if 0
	fprintf(stderr, "Reading 177130 @%06o, returned %06o\n",
		pdp.regs[PC], *word);
#endif
	break;
	case 2: /* 177132 */
		if (pdt->inprogress) {
			*word = disk_read_word(pdt);
#if 0
			fprintf(stderr, "Reading 177132 @%06o, returned %06o\n",
			pdp.regs[PC], *word);
#endif
		} else {
			static dummy = 0x5555;
			fprintf(stderr, "?");
			// fprintf(stderr, _("Reading 177132 when no I/O is progress?\n"));
			*word = dummy = ~dummy;
		}
	break;
	}
	return OK;
}

int
disk_write(c_addr addr, d_word word) {
	d_word offset = addr - DISK_REG;
	disk_t * pdt;
	switch (offset) {
	case 0:         /* control port */
		if (word) {
			/* Print a message if something other than turning
			 * all drives off is requested
			 */
			fprintf(stderr, _("Writing 177130, data %06o\n"), word);
		}
		switch (word & 0xf) {
		/* lowest bit set selects the drive */
		case 0: selected = -1; break;
		case 1: default: selected = 0; break;
		case 2: case 6: case 10: case 14: selected = 1; break;
		case 4: case 12: selected = 2; break;
		case 8: selected = 3; break;
		}
		if (selected >= 0) {
			pdt = &disks[selected];
			pdt->inprogress |= !! (word & 0400);
			fprintf(stderr, "Drive %d i/o %s\n", selected,
				pdt->inprogress ? "ON" : "OFF");
		}
		break;
	case 2:
		fprintf(stderr, _("Writing 177132, data %06o\n"), word);
		break;
	}
	return OK;
}

int
disk_bwrite(c_addr addr, d_byte byte) {
	d_word offset = addr - DISK_REG;
	switch (offset) {
	case 0:
		fprintf(stderr, _("Writing byte 177130, data %03o\n"), byte);
		break;
	case 1: 
		fprintf(stderr, _("Writing byte 177131, data %03o\n"), byte);
		break;
	case 2:
		fprintf(stderr, _("Writing byte 177132, data %03o\n"), byte);
		break;
	case 3:
		fprintf(stderr, _("Writing byte 177133, data %03o\n"), byte);
		break;
	}
	return OK;
}

#define ERRNO 052

#define ADDR 026
#define WCNT 030
#define SIDE 032
#define TRK 033
#define UNIT 034
#define SECTOR 035

void fake_disk_io() {
	d_word blkno = pdp.regs[0];
	d_word nwords = pdp.regs[1];
	c_addr ioaddr = pdp.regs[2];
	c_addr parms = pdp.regs[3];
	d_word drive;
	lc_word(parms+UNIT, &drive);
	drive &= 3;
	do_disk_io(drive, blkno, nwords, ioaddr);
}

void fake_sector_io() {
	c_addr parms = pdp.regs[3];
	d_word drive;
	d_word nwords, ioaddr, side, trk, sector;
	int blkno;
        lc_word(parms+UNIT, &drive);
	lc_word(parms+ADDR, &ioaddr);
	lc_word(parms+WCNT, &nwords);
	sector = drive >> 8;
	drive &= 3;
	lc_word(parms+SIDE, &side);
	trk = side >> 8;
	side &= 1;
	blkno = sector-1 + SECPERTRACK * (side + 2 * trk);
	do_disk_io(drive, blkno, nwords, ioaddr);
}

void do_disk_io(int drive, int blkno, int nwords, int ioaddr) {
	fprintf(stderr, _("%s block %d (%d words) from drive %d @ addr %06o\n"),
		nwords & 0100000 ? _("Writing") : _("Reading"), blkno,
		nwords & 0100000 ? -nwords & 0xffff : nwords, drive, ioaddr);
	pdp.psw |= CC_C;
	sl_byte(&pdp, ERRNO, 0);
	if (disks[drive].image == 0) {
		fprintf(stderr, _("Disk not ready\n"));
		sl_byte(&pdp, ERRNO, 6);
	} else if (blkno >= disks[drive].length / SECSIZE) {
		fprintf(stderr, _("Block number %d too large for image size %d\n"),
			blkno, disks[drive].length);
		sl_byte(&pdp, ERRNO, 5);
	} else if (nwords < 0100000) {
		/* positive nwords means "read" */
		int i;
		for (i = 0;
		i < nwords && 256 * blkno + i < disks[drive].length/2;
		i++, ioaddr += 2) {
			if (OK != sc_word(ioaddr, disks[drive].image[256 * blkno + i])) {
				fprintf(stderr, _("Read failure @ %06o\n"), ioaddr);
				sl_byte(&pdp, ERRNO, 7);
				break;
			}
		}
		if (i == nwords) pdp.psw &= ~CC_C;
	} else if (!disks[drive].ro) {
		int i;
		/* negative nwords means "write" */
		nwords = -nwords & 0xffff;
		for (i = 0;
		i < nwords && 256 * blkno + i < disks[drive].length/2;
		i++, ioaddr += 2) {
			d_word word;
                        if (OK != lc_word(ioaddr, &word)) {
                                fprintf(stderr, _("Write failure @ %06o\n"), ioaddr);
				sl_byte(&pdp, ERRNO, 7);
                                break;
                        }
			disks[drive].image[256 * blkno + i] = word; 
                }
                if (i == nwords) pdp.psw &= ~CC_C;

	} else {
		fprintf(stderr, _("Disk is read-only\n"));
		sl_byte(&pdp, ERRNO, 1);
	}
	/* make all disk I/O ops 10 ms long */
	ticks += TICK_RATE/100;

	/* Fake RTS PC */
        pop( &pdp, &pdp.regs[PC] );
	fprintf(stderr, _("Done\n"));
}
