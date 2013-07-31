#include "defines.h"
#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_mutex.h>
#include <libintl.h>
#define _(String) gettext (String)

#define SOUND_EXPONENT	(8+io_sound_freq/20000)
#define SOUND_BUFSIZE   (1<<SOUND_EXPONENT)		/* about 1/43 sec */
#define MAX_SOUND_AGE	~0	/* always play */ 

unsigned io_max_sound_age = MAX_SOUND_AGE;
unsigned io_sound_age = MAX_SOUND_AGE;	/* in io_sound_pace's since last change */
unsigned io_sound_bufsize,
	io_sound_freq = 11025,
	io_sound_pace;
double io_sound_count;
extern unsigned io_sound_val, covox_age;
extern unsigned char covox_val;
extern flag_t nflag, fullspeed;

typedef struct {
	short * buf;
	unsigned int ptr;
} sound_buf_t;

#define NUMBUF 2

sound_buf_t sound_buf[NUMBUF];

SDL_sem * sem;
 
int cur_buf;

void callback(void * dummy, Uint8 * outbuf, int len)
{
	static int cur_out_buf;
	if (SDL_SemValue(sem) == NUMBUF) {
		// Underflow: TODO fill the buffer with silence
		// fprintf(stderr, "!");
		return;
	}
	memcpy(outbuf, sound_buf[cur_out_buf].buf, len);
	cur_out_buf = (cur_out_buf + 1) % NUMBUF;
	SDL_SemPost(sem);
}

/* Called after every instruction */
sound_flush() {
	if (fullspeed && io_sound_age >= io_max_sound_age && covox_age >= io_max_sound_age) {
		/* No change in sound bit for a while, nothing to play,
		 * and drop whatever is in the buffer, 1/21 sec does not
		 * matter.
		 */
		if (sound_buf[cur_buf].ptr != 0) {
			// TODO: Fill up the buffer with silence
			// Give the buffer to the callback
			SDL_SemWait(sem);
			sound_buf[cur_buf].ptr = 0;
			cur_buf = (cur_buf + 1) % NUMBUF;
		}
		return;
	}
	while (ticks >= io_sound_count) {
		short * p =
			&sound_buf[cur_buf].buf[sound_buf[cur_buf].ptr++];
		if (io_sound_age < 1000)
			*p = io_sound_val + covox_val << 4;
		else
			*p = (covox_val << 4) + synth_next();
		io_sound_count += io_sound_pace;
		io_sound_age++;
		covox_age++;
		if (io_sound_bufsize == sound_buf[cur_buf].ptr ||
			io_sound_age == io_max_sound_age) {
			SDL_SemWait(sem);
			sound_buf[cur_buf].ptr = 0;
			cur_buf = (cur_buf + 1) % NUMBUF;
			
		}
	}
}

void sound_finish() {
	/* release the write thread so it can terminate */
	SDL_PauseAudio(1); 
	SDL_DestroySemaphore(sem);
}

SDL_AudioSpec desired;

sound_init() {
	static init_done = 0;
	int iarg, i;
	if (!nflag)
		return;
	if (fullspeed) {
		io_max_sound_age = 2 * SOUND_BUFSIZE;
		/* otherwise UINT_MAX */
	}
	if (init_done) {
		sound_buf[cur_buf].ptr = 0;
		io_sound_age = io_max_sound_age;
		return;
	}
	fprintf(stderr, _("sound_init called\n"));

	if (-1 == SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		fprintf(stderr, _("Failed to initialize audio subsystem\n"));
	}

	desired.format = 16;
	desired.channels = 1;
	desired.freq = io_sound_freq;
	desired.samples = io_sound_bufsize = SOUND_BUFSIZE;
	desired.callback = callback;
	if (-1 == SDL_OpenAudio(&desired, 0)) {
		fprintf(stderr, _("Failed to initialize sound, freq %d, %d samples\n"), io_sound_freq, SOUND_BUFSIZE);
		nflag = 0;
		return;
	}

	io_sound_pace = TICK_RATE/io_sound_freq;
	sem = SDL_CreateSemaphore(NUMBUF);

	for (i = 0; i < NUMBUF; i++) {
		sound_buf[i].ptr = 0;
		sound_buf[i].buf = malloc(io_sound_bufsize * sizeof(short));
	}
	if (!sound_buf[NUMBUF-1].buf) {
		fprintf(stderr, _("Failed to allocate sound buffers\n"));
		exit(1);
	}
	atexit(sound_finish);
	SDL_PauseAudio(0);
	init_done = 1;
}
