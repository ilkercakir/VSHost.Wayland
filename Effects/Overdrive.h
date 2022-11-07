#ifndef Overdrive_h
#define Overdrive_h

#define _GNU_SOURCE

#include "../VSEffectShared.h"
#include "BiQuad.h"

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels

	float a, l, nf, dnf;

	audioequalizer odeq;
	eqdefaults odeqdef;
	int initialized;
	int physicalwidth;
	int insamples;
}soundoverdrive;

void set_overdriveeq(eqdefaults *d);
void soundoverdrive_set(float drive, float level, soundoverdrive *o);
void soundoverdrive_reinit(float drive, float level, int eqenabled, eqdefaults *d, soundoverdrive *o);
void soundoverdrive_init(float drive, float level, int eqenabled, eqdefaults *d, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundoverdrive *o);
void soundoverdrive_add(char* inbuffer, int inbuffersize, soundoverdrive *o);
void soundoverdrive_close(soundoverdrive *o);

void aef_init(audioeffect *ae);
void aef_setparameter(audioeffect *ae, int i, float value);
float aef_getparameter(audioeffect *ae, int i);
void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void aef_reinit(audioeffect *ae);
void aef_close(audioeffect *ae);

#endif
