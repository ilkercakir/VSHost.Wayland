#ifndef AudioMicH
#define AudioMicH

#define _GNU_SOURCE 

#include <pthread.h>
#include <alsa/asoundlib.h>
#include <math.h>

#define mono 1
#define stereo 2

typedef enum
{
	MC_RUNNING = 0,
	MC_STOPPED
}micstatus;

typedef struct
{
	pthread_mutex_t *micmutex;
	char *device;
	snd_pcm_format_t format;
	unsigned int rate;
	unsigned int channels;
	snd_pcm_t *capture_handle;
	snd_pcm_hw_params_t *hw_params;
	micstatus status;
	char *micbuffer;
	int micchannels, micbuffersize, micbufferframes, micbuffersamples;
	float prescale;
	char *buffer;
	int buffersize, bufferframes, buffersamples;
	snd_pcm_uframes_t capturebuffersize;
	int nullsamples, multichannel;
	snd_pcm_sframes_t availp;
	snd_pcm_sframes_t delayp;
}microphone;

int init_audio_hw_mic(microphone *m);
void close_audio_hw_mic(microphone *m);
void init_mic(microphone *m, char* device, snd_pcm_format_t format, unsigned int rate, unsigned int channels, int outframes);
int read_mic(microphone *m);
void signalstop_mic(microphone *m);
void close_mic(microphone *m);
#endif
