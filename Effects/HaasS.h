#ifndef HaasS_h
#define HaasS_h

#define _GNU_SOURCE

#include <alsa/asoundlib.h>
#include "DelayS.h"

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels

	float millisec;
	int initialized;
	int physicalwidth, insamples;
	sounddelay haasdly;
}soundhaas;

void soundhaas_reinit(float millisec, soundhaas *h);
void soundhaas_init(float millisec, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundhaas *h);
void soundhaas_add(char* inbuffer, int inbuffersize, soundhaas *h);
void soundhaas_close(soundhaas *h);
#endif
