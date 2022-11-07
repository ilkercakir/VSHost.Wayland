#ifndef ReverbSchroeder_h
#define ReverbSchroeder_h

#define _GNU_SOURCE

#include <sqlite3.h>

#include "../VSEffectShared.h"

#include "DelayS.h"
#include "BiQuad.h"

typedef struct
{
	int reflect;
	int reverbdelaylines;
	float *reverbprimes;
	float *feedbacks;
	sounddelay *snddlyrev;
	char* bbuf;

	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	float feedback; // feedback level 0.0 .. 1.0
	float presence; // reverbation presence 0.0 .. 1.0

	float eqoctave;
	audioequalizer reverbeq;
	eqdefaults reverbeqdef;
	float alpha; // -ln(feedbacks[0])/reverbprimes[0]
	int i;
}soundreverb;

void set_reverbeq(eqdefaults *d);
void soundreverb_reinit(int reflect, int delaylines, float feedback, float presence, eqdefaults *d, soundreverb *r);
void soundreverb_init(int reflect, int delaylines, float feedback, float presence, eqdefaults *d, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundreverb *r);
void soundreverb_add(char* inbuffer, int inbuffersize, soundreverb *r);
void soundreverb_close(soundreverb *r);

void aef_init(audioeffect *ae);
void aef_setparameter(audioeffect *ae, int i, float value);
float aef_getparameter(audioeffect *ae, int i);
void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void aef_reinit(audioeffect *ae);
void aef_close(audioeffect *ae);

#endif
