/*
 * This file is part of a BK-0010/11M simulator.
 * Originally
 * Copyright 1994, Eric A. Edwards
 * After heavy modifications
 * Copyright 1995-2003 Leonid A. Broukhis
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notices appear in all copies.  Leonid A. Broukhis makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/*
 * access.c - Access routines to read and write loactions on the QBUS
 * and main memory.  
 */


#include "defines.h"
#include <libintl.h>
#define _(String) gettext (String)

/* 
 * BK-0011 has 8 8Kw RAM pages and 4 8 Kw ROM pages.
 * RAM pages 1 and 7 are video RAM.
 */
d_word ram[8][8192];
d_word rom[4][8192];
d_word system_rom[8192];
unsigned char umr[65536];

/*
 * Page mapping, per 8 Kw page. Default is a mapping mimicking BK-0010
 */
d_word * pagemap[4] = { ram[6], ram[1], rom[0], system_rom };

#define mem(x) pagemap[(x)>>14][((x) & 037777) >> 1]

/*
 * Each bit corresponds to a Kword,
 * the lowest 8 Kwords are RAM, the next 8 are screen memory,
 * the rest is usually ROM.
 */
unsigned long pdp_ram_map = 0x0000ffff;
unsigned long pdp_mem_map;

#define IS_RAM_ADDRESS(x) ((pdp_ram_map >> ((x) >> 11)) & 1)
#define IS_VALID_ADDRESS(x) ((pdp_mem_map >> ((x) >> 11)) & 1)

/*
 * The QBUS memory map.
 */
int q_null(), q_err(c_addr, d_word), q_errb(c_addr, d_byte),
 port_read(c_addr, d_word*), port_write(c_addr, d_word), port_bwrite(c_addr, d_byte);
int secret_read(c_addr, d_word*), secret_write(c_addr, d_word), secret_bwrite(c_addr, d_byte);
int force_read( c_addr, d_word*), terak_read(c_addr, d_word*);

typedef struct {
	c_addr start;
	c_addr size;
	int (*ifunc)();
	int (*rfunc)(c_addr, d_word*);
	int (*wfunc)(c_addr, d_word);
	int (*bwfunc)(c_addr, d_byte);
} pdp_qmap;

pdp_qmap qmap_bk[] = {
	{ PORT_REG, PORT_SIZE, q_null, port_read, port_write, port_bwrite },
	{ TTY_REG, TTY_SIZE, tty_init, tty_read, tty_write, tty_bwrite },
	{ IO_REG, IO_SIZE, io_init, io_read, io_write, io_bwrite },
	{ TIMER_REG, TIMER_SIZE, timer_init, timer_read, timer_write, timer_bwrite },

	/* Line registers are available even if BASIC is mapped */
	{ LINE_REG, LINE_SIZE, line_init, line_read, line_write, line_bwrite },

	/* BASIC memory is not transparently readable, thus force_read */
	{ BASIC, BASIC_SIZE, q_null, force_read, q_err, q_errb },

	/* disk ports can only be available if there is no BASIC */
	{ DISK_REG, DISK_SIZE, disk_init, disk_read, disk_write, disk_bwrite },

	{ SECRET_REG, SECRET_SIZE, q_null, secret_read, secret_write, secret_bwrite },
	{ 0, 0, 0, 0, 0, 0 }
};

tcons_read(c_addr a, d_word *d) {
	switch (a & 077) {
	case 064:
		*d = 0200;
		break; // done
	case 066:
		fprintf(stderr, "Reading %06o\n", a);
		*d = 0;
		break; // nothing
	}
	return OK;
}

tcons_write(c_addr a, d_word d) {
	switch (a & 077) {
	case 064:
		fprintf(stderr, "Writing %06o: %06o\n", a, d);
		break;
	case 066:
		// fprintf(stderr, "Writing %03o to console port %06o\n", d, a);
		if (d != '\n' && d < 32 || d >= 127)
		fprintf(stderr, "<%o>", d);
		else
		fprintf(stderr, "%c", d);
		break;
	}
	return OK;
}

pdp_qmap qmap_terak[] = {
	{ TERAK_DISK_REG, TERAK_DISK_SIZE, tdisk_init, tdisk_read,
	tdisk_write, tdisk_bwrite },
	{ 0177564, 4, q_null, tcons_read, tcons_write, tcons_write }, 
	{ 0177764, 4, q_null, tcons_read, tcons_write, tcons_write }, 
	{ 0177744, 2, q_null, port_read, port_write, port_bwrite },
	{ 0177560, 2, q_null, port_read, port_write, port_bwrite },
	{ 0173000, 0200, q_null, terak_read, q_err, q_errb },
	{ 0, 0, 0, 0, 0, 0 }
};

pdp_qmap * qmap = qmap_bk;

pdp_qmap q_printer = {
	PORT_REG, PORT_SIZE, printer_init, printer_read, printer_write, printer_bwrite
};

pdp_qmap q_mouse = {
	PORT_REG, PORT_SIZE, mouse_init, mouse_read, mouse_write, mouse_bwrite
};

pdp_qmap q_covox = {
	PORT_REG, PORT_SIZE, covox_init, covox_read, covox_write, covox_bwrite
};

pdp_qmap q_synth = {
	PORT_REG, PORT_SIZE, synth_init, synth_read, synth_write, synth_bwrite
};

pdp_qmap q_bkplip = {
	PORT_REG, PORT_SIZE, bkplip_init, bkplip_read, bkplip_write, bkplip_bwrite
};
void plug_printer() { qmap[0] = q_printer; }
void plug_mouse() { qmap[0] = q_mouse; }
void plug_covox() { qmap[0] = q_covox; }
void plug_synth() { qmap[0] = q_synth; }
void plug_bkplip() { qmap[0] = q_bkplip; }

/* When nothing is connected to the port */
int port_read(c_addr a, d_word *d) {
	*d = 0;		/* pulldown */
	fprintf(stderr, "Reading port %06o\n", a);
	return OK;
}

int port_write(c_addr a, d_word d) {
	fprintf(stderr, "Writing %06o to port %06o\n", d, a);
	return OK;	/* goes nowhere */
}

int port_bwrite(c_addr a, d_byte d) {
	fprintf(stderr, "Writing %03o to port %06o\n", d, a);
	return OK;	/* goes nowhere */
}

secret_read(addr, word)
c_addr addr;
d_word *word;
{
        d_word offset = addr - SECRET_REG;
        switch(offset) {
        case 0: /* 177700 */
                *word = 0177400;
		fprintf(stderr, "Reading 0177700\n");
                break;
        case 2: /* 177702 */
		fprintf(stderr, "Reading 0177702\n");
                *word = 0177777;
                break;
        case 4: /* 177704 */
		fprintf(stderr, "Reading 0177704\n");
		*word = 0;
        }
        return OK;
}

int secret_write(c_addr a, d_word d) {
	fprintf(stderr, "Writing %o to %o\n", d, a);
	return OK;	/* goes nowhere */
}

int secret_bwrite(c_addr a, d_byte d) {
	fprintf(stderr, "Writing %o to %o\n", d, a);
	return OK;	/* goes nowhere */
}


/*
 * lc_word() - Load a word from the given core address.
 */

int
lc_word( addr, word )
c_addr addr;
d_word *word;
{
	int i;

	addr &= ~1;

	if ( IS_VALID_ADDRESS(addr)) {
		if (!umr[addr]) {
			// fprintf(stderr, "UMR @ %06o\n", addr);
		}
		*word = mem(addr);
		return OK;
	}

	for ( i = 0; qmap[i].start; ++i ) {
		if (( addr >= qmap[i].start ) &&
		 ( addr < ( qmap[i].start + (qmap[i].size *2 )))) {
			return (qmap[i].rfunc)( addr, word );
		}
	}
	fprintf(stderr, _("Illegal read address %06o:"), addr);
	return BUS_ERROR;
}


int
force_read(c_addr addr, d_word *word )
{
        *word = mem(addr);
        return OK;
}

int
terak_read(c_addr addr, d_word *word )
{
	if (addr == 0173176) fprintf(stderr, "Reading serial num\n");
	if (addr >= 0173000 && addr < 0173200) {
        *word = mem(addr);
        return OK;
	}
	return BUS_ERROR;
}


/*
 * If the address corresponds to a video page (1 or 2)
 * returns 1 or 2, otherwise 0.
 */
unsigned char video_map[4] = { 0, 1, 0, 0};
#define VIDEO_PAGE(x) video_map[(x)>>14]
/*
 * bits 14-12 - page mapped to addresses 040000-077777
 * bits 10-8 - page mapped to addresses 100000-137777
 * bit 0 - ROM 0 mapped to addresses 100000-137777
 * bit 1 - ROM 1 mapped to addresses 100000-137777
 * bit 3 - ROM 2 mapped to addresses 100000-137777
 * bit 4 - ROM 3 mapped to addresses 100000-137777
 */
static d_word oldmap;

void pagereg_write(d_word word) {
	if (oldmap == word) return;
	oldmap = word;
	pagemap[1] = ram[(word >> 12) & 7];
	pagemap[2] = ram[(word >> 8) & 7];
	switch (word & 033) {
	case 000:
		pdp_ram_map |= 0x00ff0000;
		pdp_mem_map |= 0x00ff0000;
		break;
	case 001:
		pagemap[2] = rom[0];
		pdp_ram_map &= 0xff00ffff;
		pdp_mem_map |= 0x00ff0000;
		break;
	case 002:
		pagemap[2] = rom[1];
		pdp_ram_map &= 0xff00ffff;
		pdp_mem_map |= 0x00ff0000;
		break;
	case 010:
		pagemap[2] = rom[2];
		pdp_ram_map &= 0xff00ffff;
		pdp_mem_map &= 0xff00ffff;
		break;
	case 020:
		pagemap[2] = rom[3];
		pdp_ram_map &= 0xff00ffff;
		pdp_mem_map &= 0xff00ffff;
		break;
	default: fprintf(stderr, "Bad ROM map %o\n", word & 033);
	}

	video_map[1] = video_map[2] = 0;
	if (pagemap[1] == ram[1]) video_map[1] = 1;
	else if (pagemap[1] == ram[7]) video_map[1] = 2;
	if (pagemap[2] == ram[1]) video_map[2] = 1;
	else if (pagemap[2] == ram[7]) video_map[2] = 2;
	/* fprintf(stderr, "Pagemap = %06o\n", word); */
}

void pagereg_bwrite(d_byte byte) {
	if (oldmap >> 8 == byte) return;
	oldmap = oldmap & 0xff | byte << 8;
	pagemap[1] = ram[(byte >> 4) & 7];
	pagemap[2] = ram[byte & 7];

	video_map[1] = video_map[2] = 0;
	if (pagemap[1] == ram[1]) video_map[1] = 1;
	else if (pagemap[1] == ram[7]) video_map[1] = 2;
	if (pagemap[2] == ram[1]) video_map[2] = 1;
	else if (pagemap[2] == ram[7]) video_map[2] = 2;
	/* fprintf(stderr, "Pagemap = %06o\n", word); */
}

/*
 * sc_word() - Store a word at the given core address.
 */

int
sc_word( addr, word )
c_addr addr;
d_word word;
{
	int i;

	addr &= ~1;

	if (IS_RAM_ADDRESS(addr)) {
		if (VIDEO_PAGE(addr) && mem(addr) != word) {
			scr_write(VIDEO_PAGE(addr)-1, addr & 037777, word);
		}
		umr[addr] = 1;
		mem(addr) = word;
		return OK;
	}

	for ( i = 0; qmap[i].start; ++i ) {
		if (( addr >= qmap[i].start ) &&
		 ( addr < ( qmap[i].start + ( qmap[i].size * 2 )))) {
			return (qmap[i].wfunc)( addr, word );
		}
	}
	fprintf(stderr, _("@%06o Illegal write address %06o:"), pdp.regs[PC], addr);
	return BUS_ERROR;
}


/*
 * ll_byte() - Load a byte from the given logical address.
 * The PDP can't really do byte reads, so do a word read and
 * get the proper piece.
 */

int
ll_byte( p, baddr, byte )
register pdp_regs *p;
d_word baddr;
d_byte *byte;
{
	d_word word;
	d_word laddr;

	int i;

	/*
	 * Get the word address.
	 */

	laddr = baddr & 0177776;

	/*
	 * Address translation.
	 */

	if ((i = ll_word(p, laddr, &word)) != OK)
		return i;

	if ( baddr & 1 ) {
		word = (word >> 8) & 0377;
	} else {
		word = word & 0377;
	}

	*byte = word;

	return OK;
}


/*
 * sl_byte() - Store a byte at the given logical address.
 */

int
sl_byte( p, laddr, byte )
register pdp_regs *p;
d_word laddr;
d_byte byte;
{
	d_word t;
	int i;

	if ( IS_RAM_ADDRESS(laddr)) {
		t = mem(laddr);
		if (laddr & 1) {
			t = t & 0x00ff | byte << 8;
		} else {
			t = t & 0xff00 | byte;
		}
		if (VIDEO_PAGE(laddr) && mem(laddr) != t) {
			scr_write(VIDEO_PAGE(laddr)-1, laddr & 037776, t);
		}
		mem(laddr) = t;
		return OK;
	}

	for ( i = 0; qmap[i].start; ++i ) {
		if (( laddr >= qmap[i].start ) &&
		 ( laddr < ( qmap[i].start + ( qmap[i].size * 2 )))) {
			return (qmap[i].bwfunc)( laddr, byte );
		}
	}

	fprintf(stderr, _("Illegal byte write address %06o:"), laddr);
	return BUS_ERROR;
}


/*
 * mem_init() - Initialize the memory.
 */

mem_init()
{
	int x;
	if (terak) {
	    qmap = qmap_terak;
	    pdp_mem_map = 0x0fffffff;
	    pdp_ram_map = 0x0fffffff;
	} else if (bkmodel == 0) {
	    pdp_mem_map = 0x7fffffff;
	    pdp_ram_map = 0x0000ffff;
	    if (!rompath12 || !*rompath12) {
		/*
		 * This is BK-0010 with a disk controller and
		 * 16Kb of RAM @ 120000-157777.
		 * Correct the ROM entry in the bus map.
		 */
		for ( x = 0; qmap[x].start; ++x ) {
			if (qmap[x].start == BASIC) {
				qmap[x].start = 0160000;
				qmap[x].size = 2048; /* words */
				break;
			} 
		}
		pdp_mem_map = 0x3fffffff;
		/* Blocks at 0120000 and 0140000 are RAM */
		pdp_ram_map |= 0x0ff00000;
	    }
	} else if (bkmodel == 1) {
		pdp_mem_map = 0x3fffffff;
		pdp_ram_map = 0x0000ffff;
		/* Turn BK-0010 BASIC off */
		for ( x = 0; qmap[x].start; ++x ) {
			if (qmap[x].start == BASIC) {
				qmap[x].start = 0160000;
				qmap[x].size = 0;
				break;
			} 
		}
		if (!diskrom || !*diskrom) {
		/*
		 * This is BK-0011M without a disk controller.
		 */
			pdp_mem_map = 0x0fffffff;
		}
	}

	for ( x = 0; x < PDP_FULL_MEM_SIZE; ++x ) {
		if (IS_RAM_ADDRESS(2*x))
			mem(x) = (d_word) 0xff00;
	}
}


/*
 * q_null() - Null QBUS device switch handler.
 */

int q_null()
{
	return OK;
}

/*
 * q_err() - Returns BUS error
 */

int q_err(c_addr x, d_word y)
{
	fprintf(stderr, _("Writing to ROM addr %06o:"), x);
	return BUS_ERROR;
}
int q_errb(c_addr x, d_byte y)
{
	fprintf(stderr, _("Writing byte to ROM addr %06o:"), x);
	return BUS_ERROR;
}


/*
 * q_reset() - Reset the UNIBUS devices.
 */

q_reset()
{
	int i;

	for ( i = 0; qmap[i].start; ++i ) {
		(qmap[i].ifunc)();
	}
}

