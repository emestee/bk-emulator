#include "defines.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_mutex.h>
#include <libintl.h>
#define _(String) gettext (String)

#define SOUND_BUFSIZE   512
#define MAX_SOUND_AGE	~0	/* always play */ 

unsigned io_max_sound_age = MAX_SOUND_AGE;
unsigned io_sound_freq = 11025;
double io_sound_count;
extern unsigned io_sound_val, covox_age;
extern unsigned char covox_val;
extern flag_t nflag, fullspeed;

SDL_AudioDeviceID audioDeviceId;

/* Called after every instruction */
void sound_flush() {
	int16_t buf[SOUND_BUFSIZE * 4];
	int i = 0;

	for (; io_sound_count < ticks; ++i, io_sound_count += TICK_RATE / io_sound_freq) {
      static float hi_filt = 0.0f;
      static float lo_filt = 0.0f;
      int16_t d = 0x7fff * io_sound_val + (covox_val << 4);
      hi_filt = hi_filt + .25f * (d - hi_filt);
      lo_filt = lo_filt + .4f * (d - hi_filt - lo_filt);
   	buf[i] = lo_filt;
		covox_age++;
	}
   if (i == 0)
     return;
	SDL_QueueAudio(audioDeviceId, buf, i * sizeof(int16_t));
   static int cnt = 0;
   if (cnt++ > 10000)
     {
       cnt = 0;
       if (SDL_GetQueuedAudioSize(audioDeviceId) > 4 * SOUND_BUFSIZE)
         {
           printf("The audio queue grew too much.\n");
           SDL_ClearQueuedAudio(audioDeviceId);
         }
     }
}

void sound_finish() {
	/* release the write thread so it can terminate */
	SDL_PauseAudioDevice(audioDeviceId, 1);
}

SDL_AudioSpec desired;

void sound_init() {
	static int init_done = 0;
	if (!nflag)
		return;
	if (fullspeed) {
		io_max_sound_age = 2 * SOUND_BUFSIZE;
		/* otherwise UINT_MAX */
	}
	if (init_done) {
		return;
	}
	fprintf(stderr, _("sound_init called\n"));

	if (-1 == SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		fprintf(stderr, _("Failed to initialize audio subsystem\n"));
	}

	desired.format = AUDIO_S16;
	desired.channels = 1;
	desired.freq = io_sound_freq;
	desired.samples = SOUND_BUFSIZE;
	desired.callback = NULL;
   SDL_AudioSpec obtained;
   audioDeviceId = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

	atexit(sound_finish);
	SDL_PauseAudioDevice(audioDeviceId, 0);
	init_done = 1;
}
