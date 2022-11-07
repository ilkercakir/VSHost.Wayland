#ifndef PulseAudio_h
#define PulseAudio_h

#define _GNU_SOURCE

#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <locale.h>
#include <pthread.h>
#include <glib.h>

#include <pulse/pulseaudio.h>
//#include <pulse/i18n.h>
#include <sndfile.h>

#include <alsa/asoundlib.h>

#include "AudioPipeNB.h"

#define GETTEXT_PACKAGE "pulseaudio" 
#define PULSE_LOCALEDIR "/usr/locale" 
#define PACKAGE_VERSION ""

typedef enum
{
	PA_RUNNING,
	PA_STOPPED,
	PA_IDLE
}pastatus;

typedef struct
{
	pa_context *context;
	pa_stream *stream;
	pa_mainloop_api *mainloop_api;

	char *stream_name, *client_name, *device;

	int verbose;
	pa_volume_t volume;

	pa_sample_spec sample_spec;
	pa_channel_map channel_map;
	int channel_map_set;
	int latency;
	pa_buffer_attr bufattr;
	pa_stream_flags_t stream_flags;

	pa_mainloop* m;
	char *name, *server;
	audiopipe ap;

	cpu_set_t cpu;
	pthread_t tid;
	int retval;

	pastatus status;
}paplayer;

void init_paplayer(paplayer *p, snd_pcm_format_t format, unsigned int rate, unsigned int channels);
void paplayer_add(paplayer *p, char *inbuffer, int inbuffersize);
void paplayer_remove(paplayer *p, char *outbuffer, int outbuffersize);
void close_paplayer(paplayer *p);
#endif
