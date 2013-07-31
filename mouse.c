#include "defines.h"
#include "SDL/SDL.h"

#define LOGARITHMIC

d_word mouse_button_state;
int grab_mode;
unsigned short mouse_up, mouse_right, mouse_down,
		 mouse_left, mouse_but0, mouse_strobe;

int relx, rely;

int
mouse_init() {
	switch (mouseflag) {
	case 1: /* mouse in lower byte */
		mouse_up = 1;
		mouse_right = 2;
		mouse_down = 4;
		mouse_left = 8;
		mouse_but0 = 16;
		mouse_strobe = 8;
		break;
	case 2: /* mouse in upper byte, OS-BK11 */
		mouse_up = 0x100;
		mouse_right = 0x200;
		mouse_down = 0x400;
		mouse_left = 0x800;
		mouse_but0 = 0x1000;
		mouse_strobe = 0x8000;
		break;
	}	
}

void
mouse_event(SDL_Event * pev) {
	switch (pev->type) {
	case SDL_MOUSEBUTTONDOWN:
		if (pev->button.button == 3) {
			/* middle button switches grab mode */
			grab_mode ^= 1;
			SDL_WM_GrabInput(grab_mode ? SDL_GRAB_ON : SDL_GRAB_OFF);			return;
		}
		mouse_button_state |= mouse_but0 << pev->button.button;
		break;
	case SDL_MOUSEBUTTONUP:
		mouse_button_state &= ~(mouse_but0 << pev->button.button);
		break;
	case SDL_MOUSEMOTION:
		relx += pev->motion.xrel;
		rely += pev->motion.yrel;
		break; 
	}
}

int mouse_read(c_addr addr, d_word *word) {
	*word = mouse_button_state;
	if (relx) {
		*word |= relx > 0 ? mouse_right : mouse_left;
	}
	if (rely) {
		*word |= rely > 0 ? mouse_down : mouse_up;
	}
	/* fprintf(stderr, "Mouse %03o\n", *word); */
	return OK;
}

int mouse_write(c_addr addr, d_word word) {
	if (word & mouse_strobe)
		return OK;
	if (relx) {
#ifdef LOGARITHMIC
		relx = relx > 0 ? relx/2 : -(-relx/2);
#else
		relx = relx > 0 ? relx-1 : relx+1;
#endif
	}
	if (rely) {
#ifdef LOGARITHMIC
		rely = rely > 0 ? rely/2 : -(-rely/2);
#else
		rely = rely > 0 ? rely-1 : rely+1;
#endif
	}
	return OK;
}

mouse_bwrite(c_addr addr, d_byte byte) {
        d_word offset = addr & 1;
        d_word word;
        mouse_read(addr & ~1, &word);
        if (offset == 0) {
                word = (word & 0177400) | byte;
        } else {
                word = (byte << 8) | (word & 0377);
        }
        return mouse_write(addr & ~1, word);
}

