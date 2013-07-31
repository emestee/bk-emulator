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
 * main.c -  Main routine and setup.
 */


#include "defines.h"
#include "scr.h"
#include <SDL/SDL.h>
#include <libintl.h>
#include <locale.h>
#include <sys/time.h>
#define _(String) gettext (String)

/*
 * Globals.
 */

char * printer_file = 0;
char init_path[BUFSIZ];

/*
 * At start-up, bkmodel == 0, 1, or 2 means BK-0010, 3 means BK-0011M.
 * During simulation, bkmodel == 0 is BK-0010, 1 is BK-0011M.
 */
flag_t bkmodel = 3; /* default BK model */
flag_t terak = 0; /* by default we emulate BK */
flag_t fake_disk = 1; /* true for BK-0011M and bkmodel == 2 */

/* Standard path and ROMs for basic hardware configurations */

char * romdir = "/usr/share/bk"; /* default ROM path */
char * monitor10rom = "MONIT10.ROM";
char * focal10rom = "FOCAL10.ROM";
char * basic10rom = "BASIC10.ROM"; 
char * diskrom = "DISK_327.ROM";
char * bos11rom = "B11M_BOS.ROM";
char * bos11extrom = "B11M_EXT.ROM";
char * basic11arom = "BAS11M_0.ROM";
char * basic11brom = "BAS11M_1.ROM";

char * rompath10 = 0;
char * rompath12 = 0;
char * rompath16 = 0;

int TICK_RATE;	/* cpu clock speed */

char * floppyA = "A.img";
char * floppyB = "B.img";
char * floppyC = "C.img";
char * floppyD = "D.img";

unsigned short last_branch;
/*
 * Command line flags and variables.
 */

flag_t aflag;		/* autoboot flag */
flag_t nflag;		/* audio flag */
flag_t mouseflag;	/* mouse flag */
flag_t covoxflag;	/* covox flag */
flag_t synthflag;	/* AY-3-8910 flag */
flag_t plipflag;	/* PLIP flag */
flag_t fullspeed;	/* do not slow down to real BK speed */
flag_t traceflag;	/* print all instruction addresses */
FILE * tracefile;	/* trace goes here */
flag_t tapeflag;	/* Disable reading from tape */
flag_t turboflag;	/* "Turbo" mode with doubled clock speed */
double frame_delay;	/* Delay in ticks between video frames */
double half_frame_delay;

/*
 * main()
 */

int
main( argc, argv )
int argc;
char **argv;
{
	/* Gettext stuff */
 	setlocale (LC_ALL, "");
 	bindtextdomain ("bk", "/usr/share/locale");
 	textdomain ("bk");
	init_config();

	aflag = 1;		/* auto boot */
	nflag = 1;		/* enable sound */
	/* nothing is connected to the port by default, use ~/.bkrc */
	
	if ( args( argc, argv ) < 0 ) {
		fprintf( stderr, _("Usage: %s [options]\n"), argv[0] );
		fprintf( stderr, _("   -0        BK-0010\n") );
		fprintf( stderr, _("   -1        BK-0010.01\n") );
		fprintf( stderr, _("   -2        BK-0010.01 + FDD\n") );
		fprintf( stderr, _("   -3        BK-0011M + FDD\n") );
		fprintf( stderr, _("   -K        Terak 8510/a\n") );
		fprintf( stderr, _("   -A<file>  A: floppy image file or device (instead of %s)\n"), floppyA );
		fprintf( stderr, _("   -B<file>  B: floppy image file or device (instead of %s)\n"), floppyB );
		fprintf( stderr, _("   -C<file>  C: floppy image file or device (instead of %s)\n"), floppyC );
		fprintf( stderr, _("   -D<file>  D: floppy image file or device (instead of %s)\n"), floppyD );
		fprintf( stderr, _("   -a        Do not boot automatically\n") );
		fprintf( stderr, _("   -c        Color mode\n") );
		fprintf( stderr, _("   -n        Disable sound \n") );
		fprintf( stderr, _("   -v        Enable Covox\n") );
		fprintf( stderr, _("   -y        Enable AY-3-8910\n") );
		fprintf( stderr, _("   -m        Enable mouse\n") );
		fprintf( stderr, _("   -S        Full speed mode\n") );
		fprintf( stderr, _("   -s        \"TURBO\" mode (Real overclocked BK)\n") );
		fprintf( stderr, _("   -R<file>  Specify an alternative ROM file @ 120000.\n") );
		fprintf( stderr, _("   -r<file>  Specify an alternative ROM file @ 160000.\n") );
		fprintf( stderr, _("   -T        Disable reading from tape\n") );
		fprintf( stderr, _("   -t        Trace mode, -t<file> - to file\n") );
		fprintf( stderr, _("   -l<path>  Enable printer and specify output pathname\n") );
		fprintf( stderr, _("\n\
The default ROM files are stored in\n\
%s or the directory specified\n\
by the environment variable BK_PATH.\n"), romdir );
		fprintf( stderr, _("\nExamples:.\n") );
		fprintf( stderr, _("   'bk -R./BK.ROM' - Use custom ROM\n") );
		fprintf( stderr, _("   'bk -a -n -f'   - Developer's mode\n") );
		fprintf( stderr, _("   'bk -c'         - Gaming mode\n\n") );
		exit( -1 );
	}

	atexit(SDL_Quit);
	atexit(disk_finish);

	/* Set ROM configuration */

	if (getenv("BK_PATH"))
		romdir = getenv("BK_PATH");

	switch( bkmodel ) {
	case 0: /* BK0010 */
		rompath10 = monitor10rom;
		rompath12 = focal10rom;
		rompath16 = 0;
		TICK_RATE = 3000000;
		break;
	case 1: /* BK0010.01 */
		rompath10 = monitor10rom;
		rompath12 = basic10rom;
		rompath16 = 0;
		TICK_RATE = 3000000;
		break;
	case 2: /* BK0010.01+FDD */
		rompath10 = monitor10rom;
		rompath12 = 0;
		rompath16 = diskrom;
		TICK_RATE = 3000000;
		break;
        case 3:	/* BK-0011M */
	case 9: /* Terak 8510/a */
		rompath10 = rompath12 = rompath16 = 0;
		TICK_RATE = 4000000;
		break;
	case 4: /* Slow BK-0011M */
		rompath10 = rompath12 = rompath16 = 0;
		TICK_RATE = 3000000;
		break;
	default: /* Unknown ROM configuration */
		fprintf( stderr, _("Unknown BK model. Bailing out.\n"), argv[0] );
		exit( -1 );
	}
	
	/* Turn on the "TURBO" mode */
	if ( turboflag ) {
	    TICK_RATE = (TICK_RATE * 2); 
	}

	printf( _("Initializing SDL.\n") );

	if((SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==-1)) {
		printf( _("Could not initialize SDL: %s.\n"), SDL_GetError());
		exit(-1);
	}

	fprintf(stderr, _("Welcome to \"Elektronika BK\" emulator!\n\n") );
	showemuhelp(); /* print a short emulator help message */
	showbkhelp(); /* print a short help message */

	printf( _("SDL initialized.\n") );

	/* Convert BK model to 0010/0011 flag */
	fake_disk &= bkmodel >= 2;
	terak = bkmodel == 9;
	bkmodel = bkmodel >= 3;
	tty_open();             /* initialize the tty stuff */
	ev_init();		/* initialize the event system */
	sim_init();		/* ...the simulated cpu */
	mem_init();		/* ...main memory */
	scr_init();		/* video display */
	boot_init();		/* ROM blocks */
	if (terak) {
		// setup_terak();
	} else {
	if (mouseflag)
		plug_mouse();
	if (printer_file)
		plug_printer();
	if (covoxflag)
		plug_covox();
	if (synthflag)
		plug_synth();
	if (plipflag)
		plug_bkplip();
	}
	q_reset();             /* ...any devices */

	/* Starting frame rate */ 
	frame_delay = TICK_RATE/25;
	half_frame_delay = TICK_RATE/50;

	if (terak) {
		pdp.regs[PC] = 0173000;
	} else {
		lc_word(0177716, &pdp.regs[PC]);
		pdp.regs[PC] &= 0177400;
	}
	if (init_path[0]) {
		tracefile = fopen(init_path, "w");
	}
	if ( aflag ) {
		run( 1 );			/* go for it */	
		ui();
	} else {
		ui();				/* run the user interface */
	}

	return 0;		/* get out of here */
}


/*
 * args()
 */

args( argc, argv )
int argc;
char **argv;
{
	char *farg;
	char **narg;

	narg = argv;
	while ( --argc ) {
		narg++;
		farg = *narg;
		if ( *farg++ == '-' ) {
			switch( *farg ) {
			 case '0': case '1': case '2': case '3': case '4':
				bkmodel = *farg - '0';
				break;
			 case 'K':
				bkmodel = 9;
				// Terak has no sound yet, turn sound off
				nflag = 0;
				break;
			 case 'A':
				floppyA = *++farg ? farg : (argc--,*++narg);
				break;
			 case 'B':
				floppyB = *++farg ? farg : (argc--,*++narg);
				break;
			 case 'C':
				floppyC = *++farg ? farg : (argc--,*++narg);
				break;
			 case 'D':
				floppyD = *++farg ? farg : (argc--,*++narg);
				break;
			case 'a':
				aflag = 0;
				break;
			case 'c':
				cflag = 1;
				break;
			case 'n':
				nflag = 0;
				break;
			case 'v':
				covoxflag = 1;
				break;
			case 'y':
				synthflag = 1;
				break;
			case 'm':
				mouseflag = *(farg+1) ? *(farg+1)-'0' : 1;
				break;
			case 'p':
				plipflag = 1;
				break;
			case 'S':
				fullspeed = 1;
				break;
			case 's':
				turboflag = 1;
				break;
			case 'R':
				rompath12 = *++farg ? farg : (argc--,*++narg);
				break;
			case 'r':
				rompath16 = *++farg ? farg : (argc--,*++narg);
				break;
			case 'T':
				tapeflag = 1;
				break;
			case 't':
				traceflag = 1;
				if (*++farg)
					strcpy(init_path, farg);
				break;
			case 'l':
				printer_file = *++farg ? farg : (argc--,*++narg);;
				break;
			default:
				return -1;
				/*NOTREACHED*/
				break;
			}
		} else {
			return -1;
		}
	}
	return 0;
}


pdp_regs pdp;		/* internal processor registers */
volatile int stop_it;	/* set when a SIGINT happens during execution */


/*
 * sim_init() - Initialize the cpu registers.
 */

int
sim_init()
{
	int x;

	for ( x = 0; x < 8; ++x ) {
		pdp.regs[x] = 0;
	}
	pdp.ir = 0;
	pdp.psw = 0200;
}

/*
 * run() - Run instructions (either just one or until something bad
 * happens).  Lots of nasty stuff to set up the terminal and the
 * execution timing.
 */

int
run( int flag )
{
	register pdp_regs *p = &pdp;	/* pointer to global struct */
	struct timeval start_time;	/* system time of simulation start */
	struct timeval stop_time;	/* system time of simulation end */
	double expired, speed;		/* for statistics information */

	/*
	 * Set up the terminal cbreak i/o and start running.
	 */

	gettimeofday( &start_time, 0 );
	run_2( p, flag );
	gettimeofday( &stop_time, 0 );

	if (!flag)
		return;

	/*
	 * Compute execution statistics and print it.
	 */

	expired = ((double) stop_time.tv_sec) +
		(((double) stop_time.tv_usec) / 1000000.0 );
	expired -= ((double) start_time.tv_sec) +
		(((double) start_time.tv_usec) / 1000000.0 );
	if ( expired != 0.0 )
		speed = (((double)p->total) / expired );
	else
		speed = 0.0;
	fprintf( stderr, _("Instructions executed: %d\n"), p->total );
	fprintf( stderr, _("Simulation rate: %.5g instructions per second\n"),
		speed );
	fprintf( stderr, _("BK speed: %.5g instructions per second\n"),
		(double) p->total * TICK_RATE / ticks );
	p->total = 0;
}

double ticks_timer = 0.0;
double ticks_screen = 0.0;
extern unsigned long pending_interrupts;

int cybuf[1024];
int cybufidx = 0;

void
addtocybuf(int val) {
	cybuf[cybufidx] = val;
	cybufidx = (cybufidx+1) % 1024;
}

int
run_2( p, flag )
register pdp_regs *p;
int flag;
{
	register int result;		/* result of execution */
	int result2;			/* result of error handling */
	extern void intr_hand();	/* SIGINT handler */
	register unsigned priority;	/* current processor priority */
	int rtt = 0;			/* rtt don't trap yet flag */
	d_word oldpc;
	static char buf[80];

	/*
	 * Clear execution stop flag and install SIGINT handler.
	 */

	stop_it = 0;
	signal( SIGINT, intr_hand );

	Uint32 last_screen_update = SDL_GetTicks();
	double timing_delta = ticks - SDL_GetTicks() * (TICK_RATE/1000.0);
	c_addr startpc = p->regs[PC];

	/*
	 * Run until told to stop.
	 */

	do {
		addtocybuf(p->regs[PC]);

		/*
		 * Fetch and execute the instruction.
		 */
	
		if (traceflag) {
			extern double io_sound_count;
			disas(p->regs[PC], buf);
			if (tracefile) fprintf(tracefile, "%s\t%s\n", buf, state(p));
			else printf("%s\n", buf);
		}
		result = ll_word( p, p->regs[PC], &p->ir );
		oldpc = p->regs[PC];
		p->regs[PC] += 2;
		if (result == OK) {
			result = (itab[p->ir>>6].func)( p );
			timing(p);
		}

		/*
		 * Mop up the mess.
		 */

		if ( result != OK ) {
			switch( result ) {
			case BUS_ERROR:			/* vector 4 */
				ticks += 64;
			case ODD_ADDRESS:
				fprintf( stderr, _(" pc=%06o, last branch @ %06o\n"),
					oldpc, last_branch );
				result2 = service( (d_word) 04 );
				break;
			case CPU_ILLEGAL:		/* vector 10 */
#undef VERBOSE_ILLEGAL
#ifdef VERBOSE_ILLEGAL
				disas(oldpc, buf);
				fprintf( stderr, 
				_("Illegal inst. %s, last branch @ %06o\n"),
					buf, last_branch );
#endif
				result2 = service( (d_word) 010 );
				break;
			case CPU_BPT:			/* vector 14 */
				result2 = service( (d_word) 014 );
				break;
			case CPU_EMT:			/* vector 30 */
				result2 = service( (d_word) 030 );
				break;
			case CPU_TRAP:			/* vector 34 */
				result2 = service( (d_word) 034 );
				break;
			case CPU_IOT:			/* vector 20 */
				result2 = service( (d_word) 020 );
				break;
			case CPU_WAIT:
				in_wait_instr = 1;
				result2 = OK;
				break;
			case CPU_RTT:
				rtt = 1;
				result2 = OK;
				break;
			case CPU_HALT:
				io_stop_happened = 4;
				result2 = service( (d_word) 004 );
				break;
			default:
				fprintf( stderr, _("\nUnexpected return.\n") );
				fprintf( stderr, _("exec=%d pc=%06o ir=%06o\n"),
					result, oldpc, p->ir );
				flag = 0;
				result2 = OK;
				break;
			}
			if ( result2 != OK ) {
				fprintf( stderr, _("\nDouble trap @ %06o.\n"), oldpc);
				lc_word(0177716, &p->regs[PC]);
				p->regs[PC] &= 0177400;
				/* p->regs[SP] = 01000;	/* whatever */
			}
		}

		if (( p->psw & 020) && (rtt == 0 )) {		/* trace bit */
			if ( service((d_word) 014 ) != OK ) {
				fprintf( stderr, _("\nDouble trap @ %06o.\n"), p->regs[PC]);
				lc_word(0177716, &p->regs[PC]);
				p->regs[PC] &= 0177400;
				p->regs[SP] = 01000;	/* whatever */
			}
		}
		rtt = 0;
		p->total++;

		if (nflag)
			sound_flush();

		if (bkmodel && ticks >= ticks_timer) {
			scr_sync();
			if (timer_intr_enabled) {
				ev_register(TIMER_PRI, service, 0, 0100);
			}
			ticks_timer += half_frame_delay;
		}

		if (ticks >= ticks_screen) {
		    /* In full speed, update every 40 real ms */
		    if (fullspeed) {
			Uint32 cur_sdl_ticks = SDL_GetTicks();
		 	if (cur_sdl_ticks - last_screen_update >= 40) {
			    last_screen_update = cur_sdl_ticks;
			    scr_flush();
			}
		    } else {
			scr_flush();
		    }
		    tty_recv();
		    ticks_screen += frame_delay;
		    /* In simulated speed, if we're more than 10 ms
		     * ahead, slow down. Avoid rounding the delay up
		     * by SDL. If the sound is on, sound buffering
		     * provides synchronization.
		     */
		    if (!fullspeed && !nflag) {
		    	double cur_delta =
				ticks - SDL_GetTicks() * (TICK_RATE/1000.0);
			if (cur_delta - timing_delta > TICK_RATE/100) {
				int msec = (cur_delta - timing_delta) / (TICK_RATE/1000);
				SDL_Delay(msec / 10 * 10);
			}
		    }
		}

		/*
		 * See if execution should be stopped.  If so
		 * stop running, otherwise look for events
		 * to fire.
		 */

		if ( stop_it ) {
			fprintf( stderr, _("\nExecution interrupted.\n") );
			flag = 0;
		} else {
			priority = ( p->psw >> 5) & 7;
			if ( pending_interrupts && priority != 7 ) {
				ev_fire( priority );
			}
		}
		if (checkpoint(p->regs[PC])) {
			flag = 0;
		}
	} while( flag );

	signal( SIGINT, SIG_DFL );
}


/*
 * intr_hand() - Handle SIGINT during execution by breaking
 * back to user interface at the end of the current instruction.
 */

void intr_hand()
{
	stop_it = 1;
}

checkpoint(pc)
d_word pc;
{
    extern int breakpoint;
    extern unsigned char fake_tape;
    switch(pc) {
    case 0116256:
		if (fake_tape && !bkmodel) {
			fprintf(stderr, "Faking write file to tape\n");
			fake_write_file();
		}
		break;
    case 0116712:
		if (fake_tape && !bkmodel) {
			fprintf(stderr, _("Simulating tune-up sequence\n"));
			fake_tuneup_sequence();
		}
		break;
    case 0117260:
		if (fake_tape && !bkmodel) {
			fprintf(stderr, _("Simulating reading array with tune-up\n"));
			fake_array_with_tuneup();
		}
		break;
    case 0117376:
		if (fake_tape && !bkmodel) {
			fake_read_strobe();
		}
		break;
    case 0160250:
		if (fake_disk)
			fake_disk_io();
		break;
    case 0160372:
		if (fake_disk)
			fake_sector_io();
		break;
    case 0162246:
		fprintf(stderr, "INDEX ");
		break;
    case 0162304:
		fprintf(stderr, "err\n");
		break;
    case 0162312:
		fprintf(stderr, "good\n");
		break;
    case 0160746:
		fprintf(stderr, "WORK\n");
		break;
    case 0162012:
		fprintf(stderr, "FINDH\n");
		break;
    case 0161732:
		fprintf(stderr, "STREAD\n");
		break;
    case 0163004:
		fprintf(stderr, "GOTO00\n");
		break;
    case 0161610:
		fprintf(stderr, "RDSEC\n");
		break;
    case 0163072:
		fprintf(stderr, "FRESEC\n");
		break;
    }
    return (pc == breakpoint);
}

showemuhelp()
{
    fprintf(stderr, _("Emulator window hotkeys:\n\n"));
    fprintf(stderr, _(" ScrollLock - Toggle video mode (B/W, Color)\n"));
    fprintf(stderr, _(" Left Super+F11 - Reset emulated machine\n"));
    fprintf(stderr, _(" F12 - Load a file into BK memory\n\n"));
}

showbkhelp()
{
char *monitor10help = _("BK0010 MONITOR (the OS) commands:\n\n\
 'A' to 'K'  - Quit MONITOR.\n\
 'M'         - Read from tape. Press 'Enter' and type in the filename of\n\
               the desired .bin snapshot. Wait until the data loads into\n\
               the memory or use F12 instead.\n\
 'S'         - Start execution. You can specify an address to start from.\n");

char *monitor11help = _("BK0011M BOS commands:\n\n\
 'B'         - Boot from any bootable floppy.\n\
 'xxxB'      - Boot from the floppy drive xxx.\n\
 'L'         - Load file from tape\n\
 'xxxxxxL'   - Load file to address xxxxxx.\n\
 'M' or '0M' - Turn the tape-recoder on.\n\
 'xM'        - Turn the tape-recoder off.\n\
 'G'         - Run currently loaded program.\n\
 'xxxxxxG'   - Run from address xxxxxx.\n\
 'P'         - Continue after the STOP key press or HALT.\n\
 'Step'      - Execute a single instruction and return to MONITOR.\n\
 'Backspace' - Delete last digit (digits only).\n\
 'xxxxxx/'   - Open word xxxxxx (octal) in memory for editing.\n\
 'xxxxxx\\'   - Open byte xxxxxx (octal) in memory for editing.\n\
 'Rx'        - Open system register x for editing.\n\
 'Enter'     - Close opened memory cell and accept changes.\n\
 'Up'        - Move to the next memory cell and accept changes.\n\
 'Down'      - Move to the previous memory cell and accept changes\n\
 'Left'      - Jump to address <address>+<word>+2 (\"67\" addressing).\n\
 'Right'     - Jump to address <address>+<byte>*2+2 (assembler 'BR' jump)\n\
 '@'         - Close and jump to the address stored in the current memory cell.\n\
 'N;MC'      - Map memory page N (octal) to address range M (octal).\n");

    switch( bkmodel ) { /* Make the hints model-specific */
    case 0: /* BK0010 */
	fprintf(stderr, monitor10help);
        fprintf(stderr, _(" 'T' - Run built-in tests.\n\n"));
        fprintf(stderr, _("Type 'P M' to quit FOCAL and get the MONITOR prompt.\n"));
        fprintf(stderr, _("Type 'P T' to enter the test mode. 1-5 selects a test.\n\n"));
    break;
    case 1: /* BK0010.01 */
	fprintf(stderr, monitor10help);
        fprintf(stderr, _("\nType 'MO' to quit BASIC VILNIUS 1986 and get the MONITOR prompt.\n\n"));
    break;
    case 2: /* BK0010.01+FDD */
	fprintf(stderr, monitor10help);
        fprintf(stderr, _("\nType 'S160000' to boot from floppy A:.\n"));
        fprintf(stderr, _("The BASIC ROM is disabled.\n\n"));
    break;
    case 3: /* BK0011M+FDD */
	fprintf(stderr, monitor11help);
        fprintf(stderr, _("\nBK-0011M boots automatically from the first floppy drive available.\n\n"));
    break;
    case 4: /* BK0011M */
	fprintf(stderr, monitor11help);
    break;
    }
}
