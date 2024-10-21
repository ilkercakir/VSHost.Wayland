#ifndef VSMessage_h
#define VSMessage_h

#define _GNU_SOURCE

#include <gtk/gtk.h>

typedef struct
{
	gint context_id;
	GtkWidget *statusbar;
	char message[200];
}VSMessage;

void VStudio_init_messages(VSMessage *m, GtkWidget *w, char *s);
void VStudio_message(VSMessage *m, char *msg);
void VStudio_close_messages(VSMessage *m);
#endif
