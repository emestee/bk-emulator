#include "defines.h"

d_word timer_count, timer_setup, timer_control;
double ticks_start;
unsigned int timer_period;
#define TIM_UNKNOWN1    1
#define TIM_CONTINUOUS  2
#define TIM_ENBEND      4
#define TIM_ONCE        8
#define TIM_START      16
#define TIM_DIV16      32
#define TIM_DIV4       64
#define TIM_END       128
#include <libintl.h>
#define _(String) gettext (String)

/* Period is such that the cycle duration of the timer counter
 * with both dividers on is almost exactly 3 min (178.95 sec).
 */
#define PERIOD	128 

timer_read(addr, word)
c_addr addr;
d_word *word;
{
	d_word offset = addr - TIMER_REG;
	switch(offset) {
	case 0: /* 177706 */
		*word = timer_setup;
		break;
	case 2: /* 177710 */
		timer_check();
		*word = timer_count;
		break;
	case 4: /* 177712 */
		timer_check();
		*word = 0177400 | timer_control;
	}
	return OK;
}

timer_write(addr, word)
c_addr addr;
d_word word;
{
	d_word offset = addr - TIMER_REG;
	switch(offset) {
	case 0: /* 177706 */
		/* fprintf(stderr, "Setting timer to %d (%g s)\n",
			word, (double) word * PERIOD / 1.0e7);
		*/
		timer_setup = word;
		break;
	case 2: /* 177710 */
		fprintf(stderr, _("Writing %06o to timer counter\n"), word);
		/* timer_count = word; */
		break;
	case 4: /* 177712 */
		/* fprintf(stderr, "Timer mode %03o\n", word & 0377); */
		timer_setmode(word & 0377);
		break;
	}
	return OK;
}

timer_bwrite(addr, byte)
c_addr addr;
d_byte byte;
{
	d_word offset = addr - TIMER_REG;
	switch(offset) {
	case 0: /* 177706 */
		timer_setup = (timer_setup & 0xFF00) | byte;
		break;
	case 1: /* 177707 */
		timer_setup = (byte << 8) | (timer_setup & 0xFF);
		break;
	case 2: /* 177710 */
	case 3: /* 177711 */
		fprintf(stderr, _("Writing %03o to timer counter\n"), byte);
		/* timer_count = byte; */
		break;
	case 4: /* 177712 */
		/* fprintf(stderr, "Timer mode %03o\n", byte); */
		timer_setmode(byte);
		break;
	case 5: /* 177713 */
		break;
	}
	return OK;
}

timer_check() {
	unsigned long delta;
	if (!(timer_control & TIM_START))
		return;
	delta = (ticks - ticks_start) / timer_period;
	if (delta == 0)
		return;
	if (timer_count > delta) {
		timer_count -= delta;
		ticks_start = ticks_start + delta * timer_period;
		return;
	} else {
		if (timer_control & TIM_ENBEND) {
			timer_control |= TIM_END;
		}
		if ((timer_control & TIM_ONCE) &&
		!(timer_control & TIM_CONTINUOUS)) {
			timer_count = 0;
			timer_control &= ~TIM_START;
		} else {
			if (timer_control & TIM_CONTINUOUS)
				timer_count = -delta;
			else
				timer_count = timer_setup - delta;
			ticks_start = ticks_start + delta * timer_period;
		}
	}
}

timer_setmode(mode)
d_byte mode;
{
	if (mode & TIM_UNKNOWN1) {
		fprintf(stderr, _("Setting unknown timer mode bits\n"));
	}
	timer_period = PERIOD;
	if (mode & TIM_DIV16) {
		timer_period *= 16;
	}
	if (mode & TIM_DIV4) {
		timer_period *= 4;
	}
	if (!(timer_control & TIM_START) && (mode & TIM_START)) {
		timer_count = timer_setup;
		ticks_start = ticks;
	}
	timer_control = mode;
}

timer_init() {
	timer_control = 0177400;
	timer_count = 0177777;
	timer_setup = 0011000;
}
