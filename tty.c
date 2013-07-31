/*
 * Originally
 * Copyright 1994, Eric A. Edwards
 * After very heavy modifications
 * Copyright 1995-2003 Leonid A. Broukhis

 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Leonid A. Broukhis makes no
 * representations about the suitability of this software
 * for any purpose.  It is provided "as is" without expressed
 * or implied warranty.
 */

/* 
 * tty.c - BK-0010/11M registers 177660-177664.
 * SDL event handling should really be in another file.
 */

#include "defines.h"
#include "SDL/SDL.h"
#include "SDL/SDL_keysym.h"
#include "SDL/SDL_events.h"
#include <ctype.h>
#include <libintl.h>
#define _(String) gettext (String)

#define TTY_VECTOR      060     /* standard vector for console */
#define TTY_VECTOR2     0274    /* AR2 (ALT) vector for console */

/*
 * Defines for the registers.
 */

#define TTY_IE          0100
#define TTY_DONE        0200

/* magic numbers for special keys */

#define TTY_NOTHING     0377
#define TTY_STOP        0376
#define TTY_RESET	0375	/* "reset buton", only with AR2 */
#define TTY_AR2		0374	/* AP2 */
#define TTY_SWITCH	0373	/* video mode switch */
#define TTY_DOWNLOAD	0372	/* direct file load */
d_word tty_reg;
d_word tty_data;
d_word tty_scroll = 1330;
unsigned char key_pressed = 0100;
flag_t timer_intr_enabled = 0;
int special_keys[SDLK_LAST], shifted[256];

static tty_pending_int = 0;
unsigned long pending_interrupts;
tty_open()
{
    int i;
    /* initialize the keytables */
    for (i = 0; i < SDLK_LAST; i++) {
	special_keys[i] = -1;
    }
    special_keys[SDLK_BACKSPACE] = 030;
    special_keys[SDLK_TAB] = 011;
    special_keys[SDLK_RETURN] = 012;
    special_keys[SDLK_CLEAR] = 014;        /* sbr */

    for (i = SDLK_NUMLOCK; i <= SDLK_COMPOSE; i++)
	special_keys[i] = TTY_NOTHING;

    special_keys[SDLK_SCROLLOCK] = TTY_SWITCH;
    special_keys[SDLK_LSUPER] = TTY_AR2;
    special_keys[SDLK_LALT] = TTY_AR2;
    special_keys[SDLK_ESCAPE] = TTY_STOP;

    special_keys[SDLK_DELETE] = -1;
    special_keys[SDLK_LEFT] = 010;
    special_keys[SDLK_UP] = 032;
    special_keys[SDLK_RIGHT] = 031;
    special_keys[SDLK_DOWN] = 033;
    special_keys[SDLK_HOME] = 023;         /* vs */
    special_keys[SDLK_PAGEUP] = -1;     /* PgUp */
    special_keys[SDLK_PAGEDOWN] = -1;    /* PgDn */
    special_keys[SDLK_END] = -1;
    special_keys[SDLK_INSERT] = -1;
    special_keys[SDLK_BREAK] = TTY_STOP;
    special_keys[SDLK_F1] = 0201;          /* povt */
    special_keys[SDLK_F2] = 003;           /* kt */
    special_keys[SDLK_F3] = 0213;          /* -|--> */
    special_keys[SDLK_F4] = 026;           /* |<--- */
    special_keys[SDLK_F5] = 027;           /* |---> */
    special_keys[SDLK_F6] = 0202;          /* ind su */
    special_keys[SDLK_F7] = 0204;          /* blk red */
    special_keys[SDLK_F8] = 0200;          /* shag */
    special_keys[SDLK_F9] = 014;           /* sbr */
    special_keys[SDLK_F10] = TTY_STOP;
    special_keys[SDLK_F11] = TTY_RESET;
    special_keys[SDLK_F12] = TTY_DOWNLOAD;
    for (i = 0; i < 256; i++) {
	shifted[i] = i;
    }
    for (i = 'A'; i <= 'Z'; i++) {
	shifted[i] = i ^ 040;
	shifted[i ^ 040] = i;
    }
    shifted['1'] = '!';
    shifted['2'] = '@';
    shifted['3'] = '#';
    shifted['4'] = '$';
    shifted['5'] = '%';
    shifted['6'] = '^';
    shifted['7'] = '&';
    shifted['8'] = '*';
    shifted['9'] = '(';
    shifted['0'] = ')';
    shifted['-'] = '_';
    shifted['='] = '+';
    shifted['\\'] = '|';
    shifted['['] = '{';
    shifted[']'] = '}';
    shifted[';'] = ':';
    shifted['\''] = '"';
    shifted['`'] = '~';
    shifted[','] = '<';
    shifted['.'] = '>';
    shifted['/'] = '?';
}

/*
 * tty_init() - Initialize the BK-0010 keyboard
 */

int
tty_init()
{
	int i;
	unsigned short old_scroll = tty_scroll;
	tty_reg = 0;
	tty_data = 0;
	tty_pending_int = 0;
	tty_scroll = 01330;
	timer_intr_enabled = 0;
	if (old_scroll != tty_scroll) {
		scr_dirty = 1;
	}
}

/*
 * tty_read() - Handle the reading of a "keyboard" register.
 */

int
tty_read( addr, word )
c_addr addr;
d_word *word;
{
	d_word offset = addr & 07;

	switch( offset ) {
	case 0:
		*word = tty_reg;
		break;
	case 2:
		*word = tty_data;
		tty_reg &= ~TTY_DONE;
		break;
	case 4:
		*word = tty_scroll;
		break;
	}
	return OK;
}

/*
 * tty_write() - Handle a write to one of the "keyboard" registers.
 */

int
tty_write( addr, word )
c_addr addr;
d_word word;
{
	d_word offset = addr & 07;
	d_word old_scroll;
	char c;

	switch( offset ) {
	case 0:
		/* only let them set IE */
		tty_reg = (tty_reg & ~TTY_IE) | (word & TTY_IE);
		break;
	case 2:
		if (bkmodel) {
			flag_t old_timer_enb = timer_intr_enabled;
			scr_param_change((word >> 8) & 0xF, word >> 15);
			timer_intr_enabled = (word & 040000) == 0;
			if (timer_intr_enabled != old_timer_enb) {
				fprintf(stderr, _("Timer %s\n"), timer_intr_enabled ? _("ON") : _("OFF"));
			}
			if (!timer_intr_enabled)
				pending_interrupts &= ~(1<<TIMER_PRI);
		} else {
			fprintf(stderr, _("Writing to kbd data register, "));
			return BUS_ERROR;
		}
		break;
	case 4:
		old_scroll = tty_scroll;
		tty_scroll = word & 01377;
		if (old_scroll != tty_scroll) {
			scr_dirty = 1;
		}
		break;
	}
	return OK;
}

/*
 * kl_bwrite() - Handle a byte write.
 */

int
tty_bwrite( addr, byte )
c_addr addr;
d_byte byte;
{
	d_word offset = addr & 07;
	d_word old_scroll;
	char c;

	switch( offset ) {
	case 0:
		/* only let them set IE */
		tty_reg = (tty_reg & ~TTY_IE) | (byte & TTY_IE);
		break;
	case 1:
		break;
	case 2:
		fprintf(stderr, _("Writing to kbd data register, "));
		return BUS_ERROR;
	case 3:
		if (bkmodel) {
			flag_t old_timer_enb = timer_intr_enabled;
			scr_param_change(byte & 0xF, byte >> 7);
			timer_intr_enabled = (byte & 0100) == 0;
			if (timer_intr_enabled != old_timer_enb) {
				fprintf(stderr, "Timer %s\n", timer_intr_enabled ? "ON" : "OFF");
			}
			if (!timer_intr_enabled)
				pending_interrupts &= ~(1<<TIMER_PRI);
		} else {
			fprintf(stderr, _("Writing to kbd data register, "));
			return BUS_ERROR;
		}
		break;
	case 4:
		old_scroll = tty_scroll;
		tty_scroll = (tty_scroll & 0xFF00) | (byte & 0377);
		if (old_scroll != tty_scroll) {
			scr_dirty = 1;
		}
		break;
	case 5:
		old_scroll = tty_scroll;
		tty_scroll = (byte << 8) & 2 | tty_scroll & 0377;
		if (old_scroll != tty_scroll) {
			scr_dirty = 1;
		}
		break;
	}
	return OK;
}

/*
 * tty_finish()
 */

tty_finish( c )
unsigned char c;
{
	service(( c & 0200 ) ? TTY_VECTOR2 : TTY_VECTOR);
	tty_pending_int = 0;
}

stop_key() {
    io_stop_happened = 4;
    service(04);
}

static int ar2 = 0;
tty_keyevent(SDL_Event * pev) {
	int k, c;
	k = pev->key.keysym.sym;
	if (SDLK_UNKNOWN == k) {
	    return;
	}
	if(pev->type == SDL_KEYUP) {
	    key_pressed = 0100;
	    if (special_keys[k] == TTY_AR2) ar2 = 0;
	    return;
	}
	/* modifier keys handling */
	if (special_keys[k] != -1) {
	    switch (special_keys[k]) {
	    case TTY_STOP:
		stop_key();     /* STOP is NMI */
		return;
	    case TTY_NOTHING:
		return;
	    case TTY_AR2:
		ar2 = 1;
		return;
	    case TTY_RESET:
		if (ar2) {
			lc_word(0177716, &pdp.regs[PC]);
			pdp.regs[PC] &= 0177400;
	   	}
		return;
	    case TTY_DOWNLOAD: {
		char buf[1024];
		extern ui_load(char *);
		fputs(_("NAME? "), stderr);
		fgets(buf, 1024, stdin);
		if (buf[strlen(buf)-1] == '\n') {
			buf[strlen(buf)-1] = '\0';
		}
		ui_load(buf);
		return;
	    }
	    case TTY_SWITCH:
		scr_switch(0, 0);
		return;
	    default:
		c = special_keys[k];
	    }
	} else {
	    // Keysym follows ASCII
	    c = k;
	    if ((pev->key.keysym.mod & KMOD_CAPS) && isalpha(c)) {
		c &= ~040;  /* make it capital */
	    }
	    if (pev->key.keysym.mod & KMOD_SHIFT) {
		c = shifted[c];
	    }
	    if ((pev->key.keysym.mod & KMOD_CTRL) && (c & 0100)) {
		c &= 037;
	    }
	}
	// Win is AP2
	if (ar2) {
	    c |= 0200;
	}
	tty_reg |= TTY_DONE;
	tty_data = c & 0177;
	key_pressed = 0;
	if ( !tty_pending_int && !(tty_reg & TTY_IE) ) {
		ev_register(TTY_PRI, tty_finish, (unsigned long) 0, c);
		tty_pending_int = c & 0200 ? TTY_VECTOR2 : TTY_VECTOR;
	}
}

/*
 * tty_recv() - Called at various times during simulation to
 * set if the user typed something.
 */

tty_recv()
{
    SDL_Event ev;
    /* fprintf(stderr, "Polling events..."); */
    while (SDL_PollEvent(&ev)) {
	extern void mouse_event(SDL_Event * pev);

	    switch (ev.type) {
	    case SDL_KEYDOWN: case SDL_KEYUP:
		tty_keyevent(&ev);
		break;
	    case SDL_VIDEOEXPOSE:
	    case SDL_ACTIVEEVENT:
		/* the visibility changed */
		scr_dirty  = 256;
		break;
	    case SDL_MOUSEBUTTONDOWN:
	    case SDL_MOUSEBUTTONUP:
	    case SDL_MOUSEMOTION:
		mouse_event(&ev);
		break;
	    case SDL_VIDEORESIZE:
		scr_switch(ev.resize.w, ev.resize.h);
		break;
	    case SDL_QUIT:
		exit(0);
	    default:;
		fprintf(stderr, _("Unexpected event %d\n"), ev.type);
	    }
    }
    /* fprintf(stderr, "done\n"); */
}

