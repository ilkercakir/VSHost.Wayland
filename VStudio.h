#ifndef VStudio_h
#define VStudio_h

#define _GNU_SOURCE

#include <stdint.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <sqlite3.h>

#include "VSJam.h"

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels
	unsigned int frames;

	char dbpath[256];
	int maxchains;
	int maxeffects;

	GtkWidget *window;
	GtkWidget *vbox;

	GtkWidget *houtbox;
	GtkWidget *voutbox;

	GtkWidget *hjambox;
	GtkWidget *vjambox;

	int context_id;
	GtkWidget *statusbox;
	GtkWidget *statusbar;

	audiojam aj;
	audioout ao;
	int effid;
}virtualstudio;

void virtualstudio_init(virtualstudio *vs, int maxchains, int maxeffects, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, GtkWidget *window, char *dbpath);
void virtualstudio_close(virtualstudio *vs, int destroy);

#endif
