#ifndef Stereoizer_h
#define Stereoizer_h

#define _GNU_SOURCE

#include "../VSEffectShared.h"
#include "HaasS.h"
#include "VFO.h"

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels

	int inframes, insamples, physicalwidth;

	soundhaas h;
	char **buffer;
	soundvfo *v;
}soundstereo;

void soundstereo_init(snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundstereo *s);
void soundstereo_split(char* inbuffer, int inbuffersize, soundstereo *s);
void soundstereo_merge(char* outbuffer, soundstereo *s);
void soundstereo_addhaas(char* inbuffer, int inbuffersize, soundstereo *s);
void soundstereo_addmodulation(char* inbuffer, int inbuffersize, soundstereo *s);
void soundstereo_close(soundstereo *s);

void aef_init(audioeffect *ae);
void aef_setparameter(audioeffect *ae, int i, float value);
float aef_getparameter(audioeffect *ae, int i);
void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void aef_reinit(audioeffect *ae);
void aef_close(audioeffect *ae);

#endif
