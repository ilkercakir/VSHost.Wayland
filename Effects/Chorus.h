#ifndef Chorus_h
#define Chorus_h

#define _GNU_SOURCE

#include "../VSEffectShared.h"
#include "DelayS.h"
#include "VFO.h"

#define MAXCHORUS 3

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels

	int maxcho, initialized;
	float prescale;

	float chodelay[MAXCHORUS];
	float chofreq[MAXCHORUS];
	float chodepth[MAXCHORUS];

	sounddelay d[MAXCHORUS];
	soundvfo v[MAXCHORUS];

	char *dlbuffer[MAXCHORUS];
}soundcho;

void soundcho_init(int maxcho, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundcho *c);
void soundcho_add(char* inbuffer, int inbuffersize, soundcho *c);
void soundcho_close(soundcho *c);

void aef_init(audioeffect *ae);
void aef_setparameter(audioeffect *ae, int i, float value);
float aef_getparameter(audioeffect *ae, int i);
void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void aef_reinit(audioeffect *ae);
void aef_close(audioeffect *ae);

#endif
