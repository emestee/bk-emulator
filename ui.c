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
 * ui.c - Simulator user interface.
 */


#include "defines.h"
#include <libintl.h>
#define _(String) gettext (String)

#define DWIDTH	4
#define EMPTY	-1
#define FALSE	0
#define TRUE	1

#define isoct(c)	(((c) >= '0') && ((c) <= '7'))


int ui_done;
int breakpoint = -1;
extern int traceflag;
static char buf[BUFSIZ];

/*
 * ui() - The user interface main loop.
 */

ui()
{
	char *s;

	ui_done = 0;
	char last_cmd = 'h';
	do {
		putchar( '%' );
		putchar( ' ' );
		fflush( stdout );
		s = fgets( buf, BUFSIZ, stdin );
		if ( s ) {
			if (s[strlen(s)-1] == '\n') {
				s[strlen(s)-1] = '\0';
			}
			while( isspace( *s ))
				++s;
			if (!*s) {
				buf[0] = last_cmd;
				buf[1] = 0;
				s = buf;
			}
			switch( *s++ ) {
			case 'a':		/* show assembler code*/
				ui_asm( s );
				break;
			case 'v':	/* show instruction buffer */
				ui_viewbuf( s );
				break;
			case 'd':	/* dump memory */
				ui_dump( s );
				break;
			case 'e':	/* edit memory */
				ui_edit( s );
				break;
			case 'g':	/* go start execution */
				ui_start( s, 1 );
				break;
			case 'r':	/* register dump */
				ui_registers();
				break;
			case 's':	/* go run one intruction */
				ui_start( s, 0 );
				break;
			case 't':
				traceflag ^= 1;
				break;
			case 'l':       /* load the file */
				ui_load(s);
				break;
			case 'i':       /* fire an interrupt */
				ui_interrupt(s);
				break;
			case 'b':	/* set a breakpoint */
				ui_breakpoint(s);
				break;
			case 'q':	/* quit */
				ui_done = 1;
				break;
			case 'h':	/* show help */
			        fprintf(stderr, _("\nEmulator shell commands:\n\n"));
			        fprintf(stderr, _(" 'a' - Show assembler code ( a [start [end]] )\n"));
				fprintf(stderr, _(" 'd' - Dump memory ( d [start [end]] ) \n"));
			        fprintf(stderr, _(" 'e' - Edit memory, end with .\n"));
			        fprintf(stderr, _(" 'g' - Start execution ('g' or 'g 100000' boots the BK0010 computer)\n"));
			        fprintf(stderr, _(" 'r' - Register dump\n"));
			        fprintf(stderr, _(" 's' - Execute a single instruction\n"));
			        fprintf(stderr, _(" 't' - Toggle trace flag\n"));
			        fprintf(stderr, _(" 'l' - Load file ('l filename.bin' loads specified file)\n"));
			        fprintf(stderr, _(" 'i' - Fire an interrupt\n"));
			        fprintf(stderr, _(" 'b' - Set a breakpoint\n"));
			        fprintf(stderr, _(" '?' - Emulator help\n"));
			        fprintf(stderr, _(" 'h' - Command help\n"));
			        fprintf(stderr, _(" 'q' - Quit\n\n"));
				break;
			case '?':	/* show help */
				showemuhelp();
				showbkhelp();
			default:	/* invalid command */
				fprintf(stderr, _("Invalid command, use d, e, g, r, s, t, l, i, b, q, ? and h for help\n"));
				break;
			}
			last_cmd = s[-1];
		} else {
			ui_done = 1;	/* NULL means done */
		}
	} while( !ui_done );
}



/*
 * rd_c_addr() - Read a 16-bit core address from
 * the character buffer.
 */

char*
rd_c_addr( s, v, good )
char *s;
c_addr *v;
int *good;
{
	while( isspace( *s ))
		++s;
	*v = 0;
	*good = EMPTY;
	while( !isspace( *s ) && ( *s != '\0' )) {
		if ( *good == EMPTY )
			*good = 0;
		if ( isoct( *s )) {
			*v = ( *v << 3 ) + ( *s - '0' );
			(*good)++;
		} else {
			*good = FALSE;
			break;
		}
		++s;
	}
	*v &= 0177777;          /* mask off to only 16 bits */
	return s;
}


/*
 * rd_d_word() - Read a 16-bit data word from
 * the character buffer.
 */

char*
rd_d_word( s, v, good )
char *s;
d_word *v;
int *good;
{
	while( isspace( *s ))
		++s;
	*v = 0;
	*good = EMPTY;
	while( !isspace(*s) && ( *s != '\0' )) {
		if ( *good == EMPTY )
			*good = 0;
		if ( isoct( *s )) {
			*v = ( *v << 3 ) + ( *s - '0' );
			(*good)++;
		} else {
			*good = FALSE;
			break;
		}
		++s;
	}
	return s;
}


/*
 * ui_dump() - Hex dump of memory.
 */

ui_dump( s )
char *s;
{
	c_addr addr;
	c_addr new;
	d_word word;
	static c_addr last = 0;
	int count = 0;
	int good;

	s = rd_c_addr( s, &new, &good );
	if ( good == FALSE ) {
		fprintf(stderr, _("Bad address\n"));;
		return;
	}
	if ( good != EMPTY ) {
		addr = new;
		s = rd_c_addr( s, &new, &good );
		if ( good == FALSE ) {
			fprintf(stderr, _("Bad address\n"));;
			return;
		}
		if ( good != EMPTY ) {
			last = new;
		} else {
			last = addr + (DWIDTH * 8);
		}
	} else {
		addr = last;
		last += (DWIDTH * 8);
	}
	
	addr &= 0177777;
	last &= 0177777;

	for( ; addr != last; addr = (addr + 2) & 0177777 ) {
		if (( count % DWIDTH ) == 0 ) {
			printf( "%06o: ", addr );
		}
		if ( lc_word( addr, &word ) == OK ) {
			printf( "%06o ", word );
		} else {
			printf( "XXXXXX " );
		}
		if (( count % DWIDTH ) == ( DWIDTH - 1 )) {
			putchar( '\n' );
		}
		++count;
	}
}


/*
 * ui_edit() - Edit memory.
 */

ui_edit( s )
char *s;
{
	static c_addr addr;
	c_addr new;
	d_word word;
	int good;
	int done = 0;
	char *t;

	s = rd_c_addr( s, &new, &good );
	if ( good == FALSE ) {
		fprintf(stderr, _("Bad address\n"));
		return;
	} else if ( good != EMPTY ) {
		addr = new;
	}

	do {
		addr &= 0777777;
		printf( "%06o=", addr );
		if ( lc_word( addr, &word ) == OK ) {
			printf( "%06o ", word );
		} else {
			printf( "XXXXXX " );
		}
		fflush( stdout );
		t = fgets( buf, BUFSIZ, stdin );
		if ( t == NULL ) {
			done = 1;	/* NULL means done */
			ui_done = 1;
		} else {
			switch( *t ) {
			case '+':	/* next addr */
			case '\0': case '\n':
				addr += 2;
				break;
			case '.':	/* exit edit */
				done = 1;
				break;
			case '-':	/* previous addr */
				addr -= 2;
				break;
			case '0':	/* write data */
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				rd_d_word( t, &word, &good );
				if ( good == FALSE ) {
					fprintf(stderr, _("Bad address\n"));
				} else {
					if( sc_word( addr, word ) != OK ) {
						printf( _("write error\n") );
					}
				}
				addr += 2;
				break;
			default:	/* invalid   command */
				fprintf(stderr, _("Bad address\n"));
				break;
			}
		}
	} while ( !done );
}


/*
 * ui_start() - Start execution or single stepping.
 */

ui_start( s, n )
char *s;
int n;
{
	extern disas(c_addr pc, char * dest);
	static char buf[80];
	d_word word;
	int good;

	s = rd_d_word( s, &word, &good );
	if ( good == FALSE ) {
		fprintf(stderr, _("Bad address\n"));
		return;
	}

	if ( good != EMPTY )
		pdp.regs[PC] = word;

	run( n );
	disas(pdp.regs[PC], buf);
	puts(buf);
}

ui_interrupt(s)
char *s;
{
	int vector;
	if (1 != sscanf(s, "%o", &vector)) {
		fprintf(stderr, _("Bad vector\n"));
		return;
	}
	service (vector);
}

ui_breakpoint(s)
char *s;
{
	int addr;
	if (1 != sscanf(s, "%o", &addr)) {
		fprintf(stderr, _("Bad address\n"));
		breakpoint = -1;
		return;
	}
	breakpoint = addr;
}

/*
 * ui_registers() - Do a simple register dump.
 */

ui_registers()
{
	printf( "R0-R4: %06o %06o %06o %06o\n",
		pdp.regs[0], pdp.regs[1], pdp.regs[2], pdp.regs[3] );
	printf( "R5-R7: %06o %06o %06o %06o\n",
		pdp.regs[4], pdp.regs[5], pdp.regs[6], pdp.regs[7] );
	printf( "PSW: %06o [", pdp.psw );
	if ( pdp.psw & 010 ) putchar( 'N' );
	if ( pdp.psw & 04 ) putchar( 'Z' );
	if ( pdp.psw & 02 ) putchar( 'V' );
	if ( pdp.psw & 01 ) putchar( 'C' );
	printf( "]\n" );
}

/* load a given binary file to a given address.
 * the parameter is expected to point at <octal number> <file name>
*/

ui_load(s)
char *s;
{
	int addr = -1, len;
	d_byte c1, c2;
	FILE *f;

	fprintf(stderr, _("LOAD called\n"));
	while ( isspace (*s) ) s++;
	if (isoct(*s)) {
	    sscanf(s, "%o", &addr);
	    do s++; while (isoct(*s) || isspace (*s));
	}
	f = fopen(s, "r");
	if (!f) {
		perror(s);
		return;
	}
	c1 = getc(f);
	c2 = getc(f);
	if (-1 == addr)
	    addr = c2 << 8 | c1;
	c1 = getc(f);
	c2 = getc(f);
	len = c2 << 8 | c1;
	fprintf(stderr, _("Reading %s into %06o... "), s, addr);
	if (addr < 01000) {
	    fprintf(stderr, _("Possible start addresses:  "));
	    do {
		    c1 = getc(f);
		    c2 = getc(f);
		    fprintf(stderr, "%06o ", c2 << 8 | c1);
		    addr += 2;
		    len -= 2;
	    } while (len > 0 && addr < 01000 && !feof(f));

	}
	/* the file is in little-endian format */
	while (len > 0 && !feof(f)) {
		c1 = getc(f);
		c2 = getc(f);
		if (OK != sc_word(addr, c2 << 8 | c1)) {
		    break;
		}
		addr += 2;
		len -= 2;
	}
	fprintf(stderr, _("Done.\nLast filled address is %06o\n"), addr - 2);
	scr_dirty = 1;
	scr_flush();
}



/*
 * ui_asm() - unassembler from address.
 */

ui_asm( s )
char *s;
{
	extern disas(c_addr pc, char * dest);
	static char buf[80];
	c_addr addr;
	c_addr new;
	d_word word;
	static c_addr last = 0;
	int count = 0;
	int good;
	char last_given = 0;

	s = rd_c_addr( s, &new, &good );
	if ( good == FALSE ) {
		fprintf(stderr, _("Bad address\n"));;
		return;
	}
	if ( good != EMPTY ) {
		addr = new;
		s = rd_c_addr( s, &new, &good );
		if ( good == FALSE ) {
			fprintf(stderr, _("Bad address\n"));;
			return;
		}
		if ( good != EMPTY ) {
			last = new;
			last_given = 1;
		} 
	} else {
		addr = last;
	}

	
	addr &= 0177777;

	for( count = 0; last_given ? addr < last : count < 23; count++ ) {
			addr=disas(addr,buf); 
			puts(buf);
		}
	last = addr;
	last &= 0177777;
}

ui_viewbuf ( char * s )
{
	d_word word;
	int good;
	char buf[80];
	extern int cybuf[1024];
	extern int cybufidx;
	s = rd_d_word( s, &word, &good );
	if (good == FALSE ) {
		fprintf(stderr, _("Bad address\n"));
		return;
	}
	if ( good == EMPTY ) {
		word = 20;
	}
	for (word = (cybufidx - word) & 1023; word != cybufidx; 
		word = (word + 1) & 1023) {
		int a = cybuf[word];
		if (a >= 0) {
			disas(a, buf);
			puts(buf);
		} else if (a == -1) {
			puts("Returning from disk I/O");
		} else {
			printf("Vector %o\n", -a);
		}
	}
}
