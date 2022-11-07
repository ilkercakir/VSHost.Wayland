#ifndef Mono_h
#define Mono_h

#define _GNU_SOURCE

#include "../VSEffectShared.h"

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels

	int initialized;
	int physicalwidth;
	int insamples;
	float prescale;
}soundmono;

void soundmono_init(snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundmono *m);
void soundmono_add(char* inbuffer, int inbuffersize, soundmono *m);
void soundmono_close(soundmono *m);

void aef_init(audioeffect *ae);
void aef_setparameter(audioeffect *ae, int i, float value);
float aef_getparameter(audioeffect *ae, int i);
void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void aef_reinit(audioeffect *ae);
void aef_close(audioeffect *ae);

#endif
