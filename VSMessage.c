
/*
 * VSMessage.c
 * 
 * Copyright 2018  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include "VSMessage.h"

// Virtual studio statusbar messages

void VStudio_init_messages(VSMessage *m, GtkWidget *w, char *s)
{
	m->context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(w), "VSHost");
	m->statusbar = w;

	VStudio_message(m, s);
//	gchar *buff = g_strdup_printf("%s", m->message);
//	gtk_statusbar_push(GTK_STATUSBAR(m->statusbar), m->context_id, buff);
//	g_free(buff);
}

gboolean VStudio_message2statusbar(gpointer data)
{
	VSMessage *m = (VSMessage *)data;

	gtk_statusbar_pop(GTK_STATUSBAR(m->statusbar), m->context_id);
	gchar *buff = g_strdup_printf("%s", m->message);
	gtk_statusbar_push(GTK_STATUSBAR(m->statusbar), m->context_id, buff);
	g_free(buff);

	return FALSE;
}

void VStudio_message(VSMessage *m, char *msg)
{
	strcpy(m->message, msg);
	gdk_threads_add_idle(VStudio_message2statusbar, m);
}

void VStudio_close_messages(VSMessage *m)
{
	m->statusbar = NULL;
	m->context_id = 0;
	m->message[0] = '\0';
}
