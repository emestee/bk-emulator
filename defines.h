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
 * defines.h
 */


/*
 * Stuff to maintain compatibility across platforms.
 */

#ifndef DEFINES_INCLUDED
#define DEFINES_INCLUDED
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>		/* COMMENT for vax-bsd */
/* #include <sgtty.h> */
#include <ctype.h>
#include <fcntl.h>
#include <string.h>

/*#include <sys/select.h>	UNCOMMENT for AIX */


#if defined(sparc) || defined(m88k)	/* ADD AIX here */
#define SWAB
#endif


/*
 * Type definitions for PDP data types.
 */

typedef unsigned long c_addr;	/* core or BK Q-bus address (17 bit so far) */
typedef unsigned short l_addr;	/* logical address (16 bit) */
typedef unsigned short d_word;	/* data word (16 bit) */
typedef unsigned char d_byte;	/* data byte (8 bit) */
typedef unsigned char flag_t;	/* for boolean or small value flags */

/*
 * PDP processor defines.
 */

#define R5	5	/* return register for MARK */
#define SP	6	/* stack pointer */
#define PC	7	/* program counter */


typedef struct _pdp_regs {
	d_word regs[8];		/* general registers */
	d_byte psw;		/* processor status byte (LSI-11) */
	d_word ir;		/* current instruction register */
	d_word ea_addr;		/* stored address for dest modifying insts */
	unsigned long total;	/* count of instructions executed */
	unsigned look_time;	/* when to handle things, saves time */
} pdp_regs;


/*
 * Definitions for the memory map and memory operations.
 */


#define OK		0	/* memory and instruction results */
#define ODD_ADDRESS	1
#define BUS_ERROR	2
#define MMU_ERROR	3
#define CPU_ILLEGAL	4
#define CPU_HALT	5
#define CPU_WAIT	6
#define CPU_NOT_IMPL	7
#define CPU_TRAP	8
#define CPU_EMT		9
#define CPU_BPT		10
#define CPU_IOT		11
#define CPU_RTT		12
#define CPU_TURBO_FAIL	13

/*
 * Q-bus device addresses.
 */

#define BASIC           0120000
#define BASIC_SIZE      (24 * 512)      /* 24 Kbytes */
#define PORT_REG        0177714         /* printer, mouse, covox, ... */
#define PORT_SIZE       1
#define IO_REG          0177716         /* tape, speaker, memory */
#define IO_SIZE         1
#define TTY_REG         0177660
#define TTY_SIZE        3
#define LINE_REG        0176560
#define LINE_SIZE       4
#define TIMER_REG       0177706
#define TIMER_SIZE      3
#define DISK_REG	0177130
#define DISK_SIZE	2
#define SECRET_REG	0177700
#define SECRET_SIZE	3
#define TERAK_BOOT	0173000
#define TERAK_BSIZE	0200
#define TERAK_DISK_REG 0177000
#define TERAK_DISK_SIZE 2

#define PDP_READABLE_MEM_SIZE   (63 * 512)  /* 0 - 175777 */
#define PDP_FULL_MEM_SIZE       (64 * 512)  /* 0 - 177777 */

extern d_word rom[4][8192], ram[8][8192], system_rom[8192];
extern int boot_init(), boot_read(), boot_write(c_addr, d_word), boot_bwrite(c_addr, d_byte);
extern int scr_init(), scr_write(int, c_addr, d_word), scr_switch(int, int);
extern int tty_init(), tty_read(), tty_write(c_addr, d_word), tty_bwrite(c_addr, d_byte);
extern int io_init(), io_read(), io_write(c_addr, d_word), io_bwrite(c_addr, d_byte);
extern int disk_init(), disk_read(), disk_write(c_addr, d_word), disk_bwrite(c_addr, d_byte);
extern int tdisk_init(), tdisk_read(), tdisk_write(c_addr, d_word), tdisk_bwrite(c_addr, d_byte);
extern void disk_finish();
extern void tdisk_finish();
extern void io_read_start();
extern int timer_init(), timer_read(), timer_write(c_addr, d_word), timer_bwrite(c_addr, d_byte);
extern int line_init(), line_read(), line_write(c_addr, d_word), line_bwrite(c_addr, d_byte);
extern int printer_init(), printer_read(), printer_write(c_addr, d_word), printer_bwrite(c_addr, d_byte);
extern int mouse_init(), mouse_read(), mouse_write(c_addr, d_word), mouse_bwrite(c_addr, d_byte);
extern int covox_init(), covox_read(), covox_write(c_addr, d_word), covox_bwrite(c_addr, d_byte);
extern int synth_init(), synth_read(), synth_write(c_addr, d_word), synth_bwrite(c_addr, d_byte), synth_next(void);
extern int bkplip_init(), bkplip_read(), bkplip_write(c_addr, d_word), bkplip_bwrite(c_addr, d_byte);
extern int service(d_word);

/*
 * Defines for the event handling system.
 */

#define NUM_PRI         2

/* Timer interrupt has higher priority */
#define TIMER_PRI	0
#define TTY_PRI		1

typedef struct _event {
	int (*handler)();		/* handler function */
	d_word info;			/* info or vector number */
	double when;			/* when to fire this event */
} event;


/*
 * Instruction Table for Fast Decode.
 */

struct _itab {
	int (*func)();
};


/*
 * Global variables.
 */

extern struct timeval real_time;
extern int ui_done;
extern unsigned short last_branch;
extern pdp_regs pdp;
extern event *event_list[NUM_PRI];
extern char * printer_file;
extern char * romdir;
extern char * rompath10, *rompath12, *rompath16;
extern char * bos11rom, * diskrom, * bos11extrom, * basic11arom, * basic11brom;
extern int TICK_RATE;

extern char * floppyA, *floppyB, *floppyC, *floppyD;
extern struct _itab itab[];
extern unsigned short tty_scroll;
extern unsigned scr_dirty;
extern flag_t key_pressed;
extern flag_t in_wait_instr;
extern unsigned io_sound_val;
extern flag_t io_stop_happened;
extern int      io_tape_mode, io_tape_val, io_tape_bit;
extern flag_t cflag, mouseflag, bkmodel, terak, nflag, fullspeed;
extern double ticks, io_tape_ticks;     /* in main clock freq, integral */
extern flag_t timer_intr_enabled;

/*
 * Inline defines.
 */

/* For BK-0010 */

#define ll_word(p, a, w) lc_word(a, w)
#define sl_word(p, a, w) sc_word(a, w)

#define CC_N	010
#define CC_Z	04
#define CC_V	02
#define CC_C	01

#define CLR_CC_V()	p->psw &= ~CC_V
#define CLR_CC_C()	p->psw &= ~CC_C
#define CLR_CC_Z()	p->psw &= ~CC_Z
#define CLR_CC_N()	p->psw &= ~CC_N
#define CLR_CC_ALL()	p->psw &= ~(CC_V|CC_C|CC_Z|CC_N)

#define SET_CC_V()	p->psw |= CC_V
#define SET_CC_C()	p->psw |= CC_C
#define SET_CC_Z()	p->psw |= CC_Z
#define SET_CC_N()	p->psw |= CC_N

#define SRC_MODE	(( p->ir & 07000 ) >> 9 )
#define SRC_REG		(( p->ir & 0700 ) >> 6 )
#define DST_MODE	(( p->ir & 070 ) >> 3 )
#define DST_REG		( p->ir & 07 )

#define LSBIT	1		/*  least significant bit */

#define	MPI	0077777		/* most positive integer */
#define MNI	0100000		/* most negative integer */
#define NEG_1	0177777		/* negative one */
#define SIGN	0100000		/* sign bit */
#define CARRY   0200000		/* set if carry out */

#define	MPI_B	0177		/* most positive integer (byte) */
#define MNI_B	0200		/* most negative integer (byte) */
#define NEG_1_B	0377		/* negative one (byte) */
#define SIGN_B	0200		/* sign bit (byte) */
#define CARRY_B	0400		/* set if carry out (byte) */

#define LOW16( data )	(( data ) & 0177777 )	/* mask the lower 16 bits */
#define LOW8( data )	(( data ) & 0377 )	/* mask the lower 8 bits */

#define CHG_CC_N( d )	if ((d) & SIGN ) \
					SET_CC_N(); \
				else \
					CLR_CC_N()

#define CHGB_CC_N( d )	if ((d) & SIGN_B ) \
				SET_CC_N(); \
			else \
				CLR_CC_N()

#define CHG_CC_Z( d )	if ( d ) \
					CLR_CC_Z(); \
				else \
					SET_CC_Z()

#define CHGB_CC_Z( d )	if ( LOW8( d )) \
				CLR_CC_Z(); \
			else \
				SET_CC_Z()

#define CHG_CC_C( d )	if ((d) & CARRY ) \
					SET_CC_C(); \
				else \
					CLR_CC_C()

#define CHG_CC_IC( d )	if ((d) & CARRY ) \
					CLR_CC_C(); \
				else \
					SET_CC_C()

#define CHGB_CC_IC( d )	if ((d) & CARRY_B ) \
				CLR_CC_C(); \
			else \
				SET_CC_C()

#define CHG_CC_V( d1, d2, d3 )	\
				if ((( d1 & SIGN ) == ( d2 & SIGN )) \
				&& (( d1 & SIGN ) != ( d3 & SIGN ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHG_CC_VC( d1, d2, d3 )	\
				if ((( d1 & SIGN ) != ( d2 & SIGN )) \
				&& (( d2 & SIGN ) == ( d3 & SIGN ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHG_CC_VS( d1, d2, d3 )	\
				if ((( d1 & SIGN ) != ( d2 & SIGN )) \
				&& (( d1 & SIGN ) == ( d3 & SIGN ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHGB_CC_V( d1, d2, d3 )	\
				if ((( d1 & SIGN_B ) == ( d2 & SIGN_B )) \
				&& (( d1 & SIGN_B ) != ( d3 & SIGN_B ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHGB_CC_VC(d1,d2,d3)	\
				if ((( d1 & SIGN_B ) != ( d2 & SIGN_B )) \
				&& (( d2 & SIGN_B ) == ( d3 & SIGN_B ))) \
					SET_CC_V(); \
				else \
					CLR_CC_V()

#define CHG_CC_V_XOR_C_N()	\
				if ((( p->psw & CC_C ) && \
				   ( p->psw & CC_N )) \
				|| ((!( p->psw & CC_C )) && \
				   ( ! ( p->psw & CC_N )))) \
					CLR_CC_V(); \
				else \
					SET_CC_V()

#endif
