#ifndef AudioPipeNBH
#define AudioPipeNBH

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

typedef enum
{
	CQ_RUNNING,
	CQ_STOPPED
}audioCQstatus;

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	unsigned int frames;

	int front, rear;
	int buffersize, cqbufferframes, cqbuffersize, inbufferframes;
	char *buffer, *cqbuffer;
	pthread_mutex_t pipemutex;
	pthread_cond_t pipehighcond;
	audioCQstatus status;
}audiopipe;

void audioCQ_init(audiopipe *p, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, unsigned int cqbufferframes);
void audioCQ_add(audiopipe *p, char *inbuffer, int inbuffersize);
audioCQstatus audioCQ_remove(audiopipe *p);
audioCQstatus audioCQ_removeVB(audiopipe *p, char *outbuffer, int outbuffersize);
void audioCQ_signalstop(audiopipe *p);
void audioCQ_close(audiopipe *p);
#endif
