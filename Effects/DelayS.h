#ifndef DelayS_h
#define DelayS_h

#define _GNU_SOURCE

#include <alsa/asoundlib.h>
#include <math.h>

typedef enum
{
	DLY_ECHO,
	DLY_DELAY,
	DLY_REVERB,
	DLY_LATE,
}dly_type; /* Delay types */

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float feedback; // feedback level 0.0 .. 1.0
	float millisec; // delay in milliseconds
	int N; // parallel delays
	float prescale;

	int physicalwidth; // bits per sample
	char *fbuffer;
	int fbuffersize;
	int fbuffersamples;
	int delaysamples;
	int delaybytes;
	int insamples;

	int front, rear, readfront;
	signed short *fshort;

	dly_type delaytype;
}sounddelay;

static inline signed short sounddelay_readsample(sounddelay *s)
{
	signed short sample = s->fshort[s->readfront++];
	s->readfront%=s->fbuffersamples;

	return sample;
}

void sounddelay_reinit(int N, dly_type delaytype, float millisec, float feedback, sounddelay *s);
void sounddelay_init(int N, dly_type delaytype, float millisec, float feedback, snd_pcm_format_t format, unsigned int rate, unsigned int channels, sounddelay *s);
void sounddelay_add(char* inbuffer, int inbuffersize, sounddelay *s);
void sounddelay_close(sounddelay *s);
signed short sounddelay_readsample(sounddelay *s);
#endif
