#ifndef AudioMixerH
#define AudioMixerH

#define _GNU_SOURCE

#include <math.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

typedef struct
{
	int channelframes, channelsamples, channelbuffersize;
	int channelbuffers;
	char** channelbuffer;
	signed short **cshort;
	int front, rear;
	int readoffset;
}audiochannel;

typedef enum
{
	MX_NONBLOCKING = 0,
	MX_BLOCKING
}xblock;

typedef enum
{
	MX_RUNNING,
	MX_STOPPED
}mxstatus;

typedef struct
{
	pthread_mutex_t *xmutex; // PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t *xlowcond; // PTHREAD_COND_INITIALIZER;
	pthread_cond_t *xhighcond; // PTHREAD_COND_INITIALIZER;

	snd_pcm_format_t format;
	int rate;
	int channels;
	int frames;

	int channelcount;
	xblock blocking;

	int front, rear;
	char *outbuffer;
	int outbufferframes, outbuffersamples, outbuffersize;
	signed short *outshort;
	audiochannel **ac;
	float prescale;
	mxstatus mixerstatus;
	int activechannels;
	pthread_cond_t *xchannelcond; // PTHREAD_COND_INITIALIZER;
}audiomixer;

typedef struct
{
	int jackframes, jacksamples, jackbuffersize;
	int mxchannel;
	audiomixer *x;
	int channelbuffers;
	char **channelbuffer;
	int *front, *rear;
}audiojack;

void allocate_audiochannel(int channelbuffers, snd_pcm_format_t format, int rate, int frames, int channels, audiochannel **c);
void deallocate_audiochannel(audiochannel **c);
void init_audiomixer(int mixerchannels, xblock blocking, snd_pcm_format_t format, int rate, int frames, int channels, audiomixer *x);
int allocatechannel_audiomixer(int frames, int channelbuffers, audiomixer *x);
void deallocatechannel_audiomixer(int channel, audiomixer *x);
mxstatus readfrommixer(audiomixer *x);
float getdelay_audiomixer(audiomixer *x);
void signalstop_audiomixer(audiomixer *x);
void close_audiomixer(audiomixer *x);
void connect_audiojack(int channelbuffers, audiojack *aj, audiomixer *x);
void init_audiojack(int channelbuffers, int buffersize, audiojack *aj);
void writetojack(char* inbuffer, int inbuffersize, audiojack *aj);
void close_audiojack(audiojack *aj);
//void jack_initialize(int channelbuffers, audiojack *aj);

#endif
