#ifndef VSJam_h
#define VSJam_h

#define _GNU_SOURCE

#include <stdint.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <sqlite3.h>

#include "VSEffect.h"
#include "AudioDev.h"
#include "AudioSpk.h"
#include "AudioMixer.h"

#include "AudioEncoder.h"
#include "PulseAudio.h"

typedef struct
{
	int id;
	char name[40];
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	unsigned int frames;

	char dbpath[256];
	int maxchains;
	int maxeffects;
	GtkWidget *window;
	GtkWidget *container;
	GtkWidget *frame;
	GtkWidget *hbox;

	GtkWidget *toolbarframe;
	GtkWidget *toolbarframehbox;
	GtkWidget *toolbarhbox;
	GtkWidget *toolbar;
	GtkWidget *icon_widget;
	GtkToolItem *savechainsbutton;
	GtkToolItem *addchainbutton;
	GtkWidget *chainnameentry;

	audioeffectchain *aec;
	audiomixer *mx;

	int aeci;
}audiojam;

typedef struct auout audioout;

typedef struct
{
	audioout *ao;
	char device[32];
	unsigned int frames;

	speaker spk;

	cpu_set_t cpu;
	pthread_t tid;
	int retval;
}mxthreadparameters;

struct auout
{
	char name[40];
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	unsigned int frames;

	GtkWidget *window;
	GtkWidget *container;
	GtkWidget *outputframe;
	GtkWidget *outputhbox;
	GtkWidget *outputdevicesvbox;
	GtkWidget *outputdevices;
	GtkWidget *ledvbox;
	GtkWidget *led;
	GtkWidget *framesvbox;
	GtkWidget *frameshbox;
	GtkWidget *frameslabel;
	GtkWidget *spinbuttonvbox;
	GtkWidget *spinbutton;
	GtkWidget *recordvbox;
	GtkWidget *recordhbox;
	GtkWidget *recordlabel;
	GtkWidget *recordformatsvbox;
	GtkWidget *recordformats;
	GtkWidget *recordswitchvbox;
	GtkWidget *recordswitch;

	int mixerChannels;
	audiomixer mx;
	audiojam *aj;
	mxthreadparameters tp;

	pthread_mutex_t mxmutex;
	pthread_cond_t mxinitcond;
	int mxready;

	char *recordedfilename;
	audioencoder aen;
};

typedef enum
{
	onone,
	ohardwaredevice,
	pulseaudio
}odevicetype;

void audioout_init(audioout *ao, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, int mixerChannels, audiojam *aj, GtkWidget *container, GtkWidget *window);
void audioout_close(audioout *ao);

void audiojam_init(audiojam *aj, int maxchains, int maxeffects, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, GtkWidget *container, char *dbpath, audiomixer *mx, GtkWidget *window);
void audiojam_addchain(audiojam *aj, char *name);
void audiojam_close(audiojam *aj);
void audiojam_stopthreads(audiojam *aj);
void audiojam_startthreads(audiojam *aj);

#endif
