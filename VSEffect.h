#ifndef VSEffect_h
#define VSEffect_h

#define _GNU_SOURCE

#include <ctype.h>
#include <stdint.h>
#include <pthread.h>
#include <dlfcn.h>
#include <alsa/asoundlib.h>
#include <gtk/gtk.h>
#include <sqlite3.h>

#include "AudioMic.h"
#include "AudioMixer.h"
#include "VSTMediaPlayer.h"
#include "VSEffectShared.h"

typedef enum
{
	TH_STOPPED,
	TH_RUNNING
}threadstatus;

typedef struct
{
	audioeffectchain *aec;
	char device[32];
	unsigned int frames;

	audiojack jack;
	int channelbuffers;
	
	microphone mic;
	vpwidgets vpw;
	playlistparams plparams;

	threadstatus status;
	cpu_set_t cpu;
	pthread_t tid;
	int retval;
}threadparameters;

typedef struct
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *toolbar;
	GtkWidget *icon_widget;
	GtkToolItem *addbutton;
	GtkToolItem *deletebutton;
	GtkWidget *scrolled_window;
	GtkWidget *view;
	GtkListStore *store;
	GtkTreeIter iter;
}audioeffectchainutil;

struct audioeffchain
{
	int id;
	char name[40];
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	unsigned int frames;

	pthread_mutex_t rackmutex;
	int effects;
	GtkWidget *container;
	GtkWidget *inputframe;
	GtkWidget *inputdevices;
	GtkWidget *inputhbox;
	GtkWidget *led;

	GtkWidget *toolbar;
	GtkWidget *icon_widget;
	GtkToolItem *addeffectbutton;
	GtkToolItem *saveeffectsbutton;
	GtkToolItem *deletechainbutton;

	GtkWidget *frame;
	GtkWidget *vbox;

	audioeffect *ae;
	int *aeorder;
	threadparameters tp;
	audiomixer *mx;

	char dbpath[256];
	audioeffectchainutil util;
	int effid;

	int channelbuffers;
};

typedef struct
{
	audioeffect *ae;
	int i;
	float value;
}audioeffectidle;

typedef enum
{
	none,
	hardwaredevice,
	mediafiledevice
}devicetype;

void audioeffect_init(audioeffect *ae, int id);
void audioeffect_setparameter(audioeffect *ae, int i, float value);
float audioeffect_getparameter(audioeffect *ae, int i);
void audioeffect_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void audioeffect_close(audioeffect *ae);

void audioeffectchain_create_thread(audioeffectchain *aec, char *device, unsigned int frames, int channelbuffers, audiomixer *mx);
void audioeffectchain_create_thread_ffmpeg(audioeffectchain *aec, char *device, unsigned int frames, int channelbuffers, audiomixer *mx);
void audioeffectchain_terminate_thread(audioeffectchain *aec);
void audioeffectchain_terminate_thread_ffmpeg(audioeffectchain *aec);
void audioeffectchain_save(audioeffectchain *aec);
void audioeffectchain_init(audioeffectchain *aec, char *name, int id, audiomixer *mx, GtkWidget *container, char *dbpath);
int audioeffectchain_loadeffect(audioeffectchain *aec, int id, char *path);
void audioeffectchain_process(audioeffectchain *aec, char *inbuffer, int inbuffersize);
void audioeffectchain_unloadeffect(audioeffectchain *aec, int effect);
void audioeffectchain_order(audioeffectchain *aec, int effect);
void audioeffectchain_unorder(audioeffectchain *aec, int effect);
void audioeffectchain_close(audioeffectchain *aec);
gboolean audioeffectchain_led(gpointer data);

devicetype get_devicetype(char *device);
#endif
