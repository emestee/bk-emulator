/*
 * Configuration file management routines.
 * Copyright 2003 Leonid Broukhis
 */
#include <stdio.h>
#include <string.h>
#include "conf.h"
#include "scr.h"
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)

typedef struct {
	char * name;
	int * val;
	int llim, ulim;	// equal means no limit
} iconf_t;

typedef struct {
	char * name;
	flag_t * val;
} bconf_t;

typedef struct {
	char * name;
	char ** val;
} sconf_t;

/*
 * Attribute names are case-insensitive.
 */
sconf_t sconf[] = {
	{ "floppyA", &floppyA },
	{ "floppyB", &floppyB },
	{ "floppyC", &floppyC },
	{ "floppyD", &floppyD },
	{ "monit10.rom", &monitor10rom }
};

iconf_t iconf[] = {
	{ "VertSize", &vertsize, 256, 1024 },
	{ "HorizSize", &horsize, 256, 1024 },
	{ "UpperPorch", &upper_porch, 0, 30 },
	{ "LowerPorch", &lower_porch, 0, 30 },
	{ "SoundFreq", &io_sound_freq, 8000, 44100 }
};

bconf_t bconf[] = {
	{ "Color", &cflag },
	{ "FakeDisk", &fake_disk },
	{ "FakeTape", &fake_tape },
	{ "FullScreen", &fullscreen },
	{ "Telegraph", &telegraph_enabled }
};

#define NUM_SATTR (sizeof(sconf)/sizeof(sconf_t))
#define NUM_IATTR (sizeof(iconf)/sizeof(iconf_t))
#define NUM_BATTR (sizeof(bconf)/sizeof(bconf_t))

init_config() {
	FILE * bkrc = popen("cat $HOME/.bkrc", "r");
	char buf[1024];
	char name[1024];
	char sval[1024];
	int ival;
	if (!bkrc) return;
	int errors = 0;
	while (fgets(buf, 1024, bkrc)) {
		int n, i;
		/* # in the first non-blank position marks a comment */
		n = strspn(buf, " \t\n");
		if (buf[n] == '\0' || buf[n] == '#')
			continue;
		
		if (1 > sscanf(buf, " %[^= ] =%n", name, &n)) {
			errors++;
			fprintf(stderr, _("Bad configuration line: %s\n"), buf);
			continue;
		}
		
		for (i = 0; i < NUM_SATTR; i++) {
			if (!strcasecmp(name, sconf[i].name)) {
				n = sscanf(buf+n, " %s", sval);
				if (n < 1) {
					errors++;
					fprintf(stderr, _("String value for %s is required\n"),
						sconf[i].name);
					break;
				}
				*sconf[i].val = strdup(sval);
				break;
			}
		}
		if (i != NUM_SATTR) continue;

		for (i = 0; i < NUM_IATTR; i++) {
			if (!strcasecmp(name, iconf[i].name)) {
				n = sscanf(buf+n, " %d", &ival);
				if (n < 1) {
					errors++;
					fprintf(stderr, _("Integer value for %s is required\n"),
						iconf[i].name);
					break;
				}
				if (iconf[i].llim != iconf[i].ulim &&
				    (ival < iconf[i].llim || ival > iconf[i].ulim)) {
					errors++;
					fprintf(stderr, _("Value of %s must be in range [%d:%d]\n"),
						iconf[i].name, iconf[i].llim, iconf[i].ulim);
					break;
				}	
				*iconf[i].val = ival;
				break;
			}
		}
		if (i != NUM_IATTR) continue;
		for (i = 0; i < NUM_BATTR; i++) {
			if (!strcasecmp(name, bconf[i].name)) {
				n = sscanf(buf+n, " %s", sval);
				if (n < 1) {
					errors++;
					fprintf(stderr, _("Boolean value for %s is required\n"),
						bconf[i].name);
					break;
				}
				switch (sval[0]) {
				case '1':
				case 'T':
				case 't':
				case 'Y':
				case 'y':
					*bconf[i].val = 1;
					break;
				case '0':
				case 'F':
				case 'f':
				case 'N':
				case 'n':
					*bconf[i].val = 0;
					break;
				default:
					errors++;
					fprintf(stderr, _("Boolean value for %s is required (got %c)\n"),
						 bconf[i].name, ival);
				}
				break;
			}
		}
		if (i != NUM_BATTR) continue;
		errors++;
		fprintf(stderr, _("Unknown attribute %s\n"), name);
	}
	pclose(bkrc);
	if (errors) {
		fprintf(stderr, _("There were %d errors in the configuration file, aborting.\n"), errors);
		exit(1);
	}
}

