#ifndef VSTMediaPlayerH
#define VSTMediaPlayerH

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h> 
#include <unistd.h>
#include <wchar.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <sqlite3.h>

#include "AudioMixer.h"
#include "VSTVideoPlayer.h"

typedef enum
{
	text_html,
	string
}target_info;

typedef enum
{
	PLIDLE = 0,
	PLPLAYING
}plstatus;

typedef struct
{
GtkWidget *vpwindow;
GtkWidget *box1;
GtkWidget *drawingarea;
GtkWidget *button_box;
GtkWidget *button1;
GtkWidget *button2;
GtkWidget *button8;
GtkWidget *button9;
GtkWidget *button10;
GtkWidget *button11;
GtkAdjustment *hadjustment;
GtkWidget *hscale;
GtkWidget *stackswitcher;
GtkWidget *stack;
GtkWidget *playerbox;
GtkWidget *box2;
GtkWidget *playlistbox;
GtkWidget *button_box2;
GtkWidget *button3;
GtkWidget *button4;
GtkWidget *button6;
GtkWidget *button7;
GtkWidget *listview;
GtkListStore *store;
GtkTreeIter iter;
GtkWidget *scrolled_window;
GtkWidget *statusbar;
gint context_id;

GtkWidget *notebook;
GtkWidget *nbpage1;
GtkWidget *nbpage2;

int hscaleupd, sliding, playerWidth, playerHeight;

videoplayer vp;
audiopipe ap;

int last_id;
pthread_t tid;
int retval0;
cpu_set_t cpu[4];
char msg[300];
GtkTargetEntry target_entries[3];
}vpwidgets;

typedef struct
{
vpwidgets *vpw;
int vqMaxLength, thread_count;
pthread_attr_t attr;
snd_pcm_format_t format;
unsigned int rate, channels;
unsigned int frames, cqbufferframes;
plstatus status;
pthread_mutex_t statusmutex;
pthread_cond_t statuscond;
}playlistparams;

void init_playlistparams(playlistparams *plparams, vpwidgets *vpw, int vqMaxLength, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, unsigned int cqbufferframes, int thread_count);
void close_playlistparams(playlistparams *plparams);
void toggle_vp(vpwidgets *vpw, GtkWidget *togglebutton);
void init_videoplayerwidgets(playlistparams *plp, int playWidth, int playHeight);
void close_videoplayerwidgets(playlistparams *plp);
void press_vp_stop_button(playlistparams *plp);
void press_vp_resume_button(playlistparams *plp);
char* strlastpart(char *src, char *search, int lowerupper);
#endif
