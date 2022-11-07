#ifndef AudioSpkH
#define AudioSpkH

#define _GNU_SOURCE 

#include <pthread.h>
#include <alsa/asoundlib.h>

typedef struct
{
	snd_pcm_uframes_t persize;
	int bufsize;
	snd_pcm_t *handle;
	char device[32];	// playback device
	unsigned int rate;	// stream rate
	snd_pcm_format_t format; // = SND_PCM_FORMAT_S16, sample format
	unsigned int channels;		// count of channels
	int resample;	// = 1, enable alsa-lib resampling
	int period_event;	// = 0, produce poll event after each period
	snd_pcm_access_t access; // = SND_PCM_ACCESS_RW_INTERLEAVED
	snd_pcm_sframes_t buffer_size;
	snd_pcm_sframes_t period_size;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_sframes_t availp;
	snd_pcm_sframes_t delayp;
}speaker;

int set_hwparams(speaker *s);
int init_audio_hw_spk(speaker *s);
void close_audio_hw_spk(speaker *s);
void init_spk(speaker *s, char* device, snd_pcm_format_t format, unsigned int rate, unsigned int channels);
void write_spk(speaker *s, char* buffer, int bufferframes);
void close_spk(speaker *s);

#endif
