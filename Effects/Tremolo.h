#ifndef Tremolo_h
#define Tremolo_h

#define _GNU_SOURCE

#include "../VSEffectShared.h"

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float depth; // depth 0.0 .. 1.0
	float tremolorate; // tremolo rate  0.1 .. 10
	int invert; // invert depth phase between L/R
	unsigned int framecount; // time=framecount/rate % rate; instantaneous volume=1-depth/2*(1-sin(2*pi*tremolorate*time))
	int framesinT, frames;

	int enabled;
	int initialized;
}soundtremolo;

void soundtremolo_reinit(int enabled, float tremolorate, float depth, int invert, soundtremolo *t);
void soundtremolo_init(int enabled, float tremolorate, float depth, int invert, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundtremolo *t);
void soundtremolo_add(char* inbuffer, int inbuffersize, soundtremolo *t);
void soundtremolo_close(soundtremolo *t);

void aef_init(audioeffect *ae);
void aef_setparameter(audioeffect *ae, int i, float value);
float aef_getparameter(audioeffect *ae, int i);
void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void aef_reinit(audioeffect *ae);
void aef_close(audioeffect *ae);

#endif
