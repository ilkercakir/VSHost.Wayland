#ifndef VFO_H
#define VFO_H

#define _GNU_SOURCE

#include <math.h>
#include <alsa/asoundlib.h>

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float vfofreq; // modulation frequency
	float vfodepth; // modulation depth in percent 0..1.0

	int N; // extra frames
	char *vfobuf;
	int physicalwidth;
	int vfobufframes;
	int vfobufsamples;
	int framebytes;
	int inbuffersamples;
	int inbufferframes;
	int front, rear;
	int framesinT, period, periods, peak, L, M;
	char *lastframe;
}soundvfo;

void soundvfo_init(float vfofreq, float vfodepth, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundvfo *v);
void soundvfo_add(char* inbuffer, int inbuffersize, soundvfo *v);
void soundvfo_close(soundvfo *v);
#endif
