#ifndef FoldingDistortion_h
#define FoldingDistortion_h

#define _GNU_SOURCE

#include "../VSEffectShared.h"

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float threshold;
	float gain;

	int physicalwidth, insamples, initialized;
}soundfoldingdistortion;

void soundfoldingdistort_init(float threshold, float gain, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundfoldingdistortion *d);
void soundfoldingdistort_add(char* inbuffer, int inbuffersize, soundfoldingdistortion *d);
void soundfoldingdistort_close(soundfoldingdistortion *d);

void aef_init(audioeffect *ae);
void aef_setparameter(audioeffect *ae, int i, float value);
float aef_getparameter(audioeffect *ae, int i);
void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void aef_reinit(audioeffect *ae);
void aef_close(audioeffect *ae);

#endif
