#ifndef VSEffectShared_h
#define VSEffectShared_h

#define _GNU_SOURCE

#include <stdint.h> 
#include <math.h> 
#include <alsa/asoundlib.h>
#include <gtk/gtk.h>

typedef enum
{
	pt_none,
	pt_switch,
	pt_scale,
	pt_check,
	pt_spin,
	pt_combo,
}parametertype;

typedef struct
{
	void *parent;
	int index;
	char name[10];
	float minval, maxval, value, step;
	int resetrequired;
	parametertype ptype;
	GtkWidget *vbox;
	GtkWidget *swbox;
	GtkWidget *pwidget;
	GtkWidget *label;
	GtkAdjustment *adj;
	char confpath[256];
}audioeffectparameter;

typedef struct audioeffchain audioeffectchain;

typedef struct audioeff audioeffect;

struct audioeff
{
	int id;
	char sopath[256];
	void *handle;

	char name[40];
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels

	void *parent; // audioeffectchain
	int index;

	int parameters;
	audioeffectparameter *parameter;
	pthread_mutex_t effectmutex;
	GtkWidget *container;
	GtkWidget *frame;
	GtkWidget *hbox;

	GtkWidget *vtoolbox;
	GtkWidget *toolbar;
	GtkWidget *icon_widget;
	GtkToolItem *removeeffectbutton;
	GtkWidget *icon_up_widget;
	GtkToolItem *moveeffectupbutton;
	GtkWidget *icon_down_widget;
	GtkToolItem *moveeffectdownbutton;

	void (*aef_init)(audioeffect*); // init
	void (*aef_setparameter)(audioeffect*, int, float); // setparameter
	float (*aef_getparameter)(audioeffect*, int); // getparameter
	void (*aef_process)(audioeffect*, uint8_t*, int); // process
	void (*aef_reinit)(audioeffect*); // reinit
	void (*aef_close)(audioeffect*); // close

	void *data;
};

void audioeffect_allocateparameters(audioeffect *ae, int count);
void audioeffect_initparameter(audioeffect *ae, int i, char* name, float minval, float maxval, float value, float step, int resetrequired, parametertype ptype);
void audioeffect_setdependentparameter(audioeffect *ae, int i, float value);
void audioeffect_deallocateparameters(audioeffect *ae);
#endif
