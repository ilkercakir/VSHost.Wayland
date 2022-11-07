#ifndef Modulation_h
#define Modulation_h

#define _GNU_SOURCE

#include "../VSEffectShared.h"
#include "VFO.h"

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float modfreq; // modulation frequency
	float moddepth; // modulation depth in percent 0..1.0

	soundvfo v;
}soundmod;

void soundmod_reinit(float modfreq, float moddepth, soundmod *m);
void soundmod_init(float modfreq, float moddepth, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundmod *m);
void soundmod_add(char* inbuffer, int inbuffersize, soundmod *m);
void soundmod_close(soundmod *m);

void aef_init(audioeffect *ae);
void aef_setparameter(audioeffect *ae, int i, float value);
float aef_getparameter(audioeffect *ae, int i);
void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void aef_reinit(audioeffect *ae);
void aef_close(audioeffect *ae);

#endif
