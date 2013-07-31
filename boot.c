/*
 * This file is part of 'pdp', a PDP-11 simulator.
 *
 * For information contact:
 *
 *   Computer Science House
 *   Attn: Eric Edwards
 *   Box 861
 *   25 Andrews Memorial Drive
 *   Rochester, NY 14623
 *
 * Email:  mag@potter.csh.rit.edu
 * FTP:    ftp.csh.rit.edu:/pub/csh/mag/pdp.tar.Z
 * 
 * Copyright 1994, Eric A. Edwards
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Eric A. Edwards makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/*
 * boot.c - Boot Code
 */

#include "defines.h"
#include <libintl.h>
#define _(String) gettext (String)

/*
 * load_rom() - Read the contents of the ROM into the array. 
 * Used for BK-0010 style ROM - stores into the mapped memory.
 */

void load_rom(unsigned start, char * rompath, unsigned min_size, unsigned max_size) {
	char * path;
	int i;
	extern unsigned long pdp_ram_map;
	unsigned long saved_ram_map = pdp_ram_map;
	FILE * romf;

	if (!rompath || !*rompath) return;
	path = malloc(strlen(romdir)+strlen(rompath)+2);

	if (!path) { fprintf(stderr, _("No memory\n")); exit(1); }

	/* If rompath is a real path, do not apply romdir to it */
	if (*romdir && !strchr(rompath, '/'))
		sprintf(path, "%s/%s", romdir, rompath);
	else
		strcpy(path, rompath);

	fprintf(stderr, _("Loading %s..."), path);

	romf = fopen(path, "r");
	if (!romf) {
		fprintf(stderr, _("Couldn't open file.\n"));
		exit(1);
	}
	pdp_ram_map = ~0l;
	for (i = 0; i < max_size/2; i++, start+=2) {
		int lobyte = getc(romf);
		int hibyte = getc(romf);
		d_word data;
		if (hibyte < 0) break;
		data = lobyte | hibyte<<8;
		sc_word(start, data);
	}
	if (i < min_size/2) {
		fprintf(stderr, _("Incomplete or damaged file.\n"));
		exit(1);
	}
	fclose(romf);
	free(path);
        fprintf(stderr, _("Done.\n"));
	pdp_ram_map = saved_ram_map;
}

/*
 * Loads BK-0011 ROM into the givem ROM block from a given offset.
 */
void load_rom11(d_word * rom, int byte_off, char * rompath, int byte_size) {
	char * path;
	int i;

	if (!rompath || !*rompath) return;
	path = malloc(strlen(romdir)+strlen(rompath)+2);

	if (!path) { fprintf(stderr, _("No memory\n")); exit(1); }

	/* If rompath is a real path, do not apply romdir to it */
	if (*romdir && !strchr(rompath, '/'))
		sprintf(path, "%s/%s", romdir, rompath);
	else
		strcpy(path, rompath);

	fprintf(stderr, _("Loading %s..."), path);

	FILE * romf = fopen(path, "r");
	if (!romf) {
		fprintf(stderr, _("Couldn't open file.\n"));
		exit(1);
	}
	rom += byte_off/2;
	for (i = 0; i < byte_size/2; i++, rom++) {
		int lobyte = getc(romf);
		int hibyte = getc(romf);
		d_word data;
		if (hibyte < 0) break;
		data = lobyte | hibyte<<8;
		*rom = data;
	}
	if (i < byte_size/2) {
		fprintf(stderr, _("Incomplete or damaged file.\n"));
		exit(1);
	}
	fclose(romf);
	free(path);
        fprintf(stderr, _("Done.\n"));
}

int
boot_init()
{
	static unsigned char boot_done = 0;
	if (boot_done) return;

	boot_done = 1;

	if (terak) {
		/* So far we only have Terak boot ROM */
		load_rom(0173000, "TERAK.ROM", 128, 128);
		return;
	}
	if (bkmodel != 0) {
		load_rom11(system_rom, 0, bos11rom, 8192);
		load_rom11(system_rom, 8192, diskrom, 4096);
		load_rom11(rom[0], 0, basic11arom, 16384);
		load_rom11(rom[1], 0, basic11brom, 8192);
		load_rom11(rom[1], 8192, bos11extrom, 8192);
		return;
	}

	/* Monitor must be exactly 8k */
	load_rom(0100000, rompath10, 8192, 8192);

        /* Basic or Focal ROM may be 24448 to 24576 bytes */
        load_rom(0120000, rompath12, 24448, 24576);

	/* Disk controller BIOS is exactly 4k */
        load_rom(0160000, rompath16, 4096, 4096);
}
