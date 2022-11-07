/*
 * VStudio.c
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

#include "VStudio.h"

// Virtual studio runtime

static void push_message(GtkWidget *widget, gint cid, char *msg)
{
  gchar *buff = g_strdup_printf("%s", msg);
  gtk_statusbar_push(GTK_STATUSBAR(widget), cid, buff);
  g_free(buff);
}

static void pop_message(GtkWidget *widget, gint cid)
{
  gtk_statusbar_pop(GTK_STATUSBAR(widget), cid);
}

void virtualstudio_init(virtualstudio *vs, int maxchains, int maxeffects, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, GtkWidget *window, char *dbpath)
{
	vs->maxchains = maxchains;
	vs->maxeffects = maxeffects;
	vs->format = format;
	vs->rate = rate;
	vs->channels = channels;
	vs->frames = frames;
	strcpy(vs->dbpath, dbpath);
	vs->window = window;

// vertical box
	vs->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(window), vs->vbox);

// horizontal box
	vs->houtbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vs->vbox), vs->houtbox);

// vertical box
	vs->voutbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(vs->houtbox), vs->voutbox);

// Audio Out Mixer
	audioout_init(&(vs->ao), vs->format, vs->rate, vs->channels, vs->frames, vs->maxchains, &(vs->aj), vs->voutbox, vs->window);

// horizontal box
	vs->hjambox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	//gtk_container_add(GTK_CONTAINER(vs->vbox), vs->hjambox);
	gtk_box_pack_start(GTK_BOX(vs->vbox), vs->hjambox, TRUE, TRUE, 0);

// vertical box
	vs->vjambox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(vs->hjambox), vs->vjambox);
	gtk_box_pack_start(GTK_BOX(vs->hjambox), vs->vjambox, TRUE, TRUE, 0);

// Audio Input Jam
	audiojam_init(&(vs->aj), vs->maxchains, vs->maxeffects, vs->format, vs->rate, vs->channels, vs->frames, vs->vjambox, vs->dbpath, &(vs->ao.mx), vs->window);

// horizontal box
	vs->statusbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(vs->vbox), vs->statusbox);
	//gtk_box_pack_start(GTK_BOX(vs->vbox), vs->statusbox, TRUE, TRUE, 0);

// Statusbar
	vs->context_id = 0;
	vs->statusbar = gtk_statusbar_new();
	gtk_container_add(GTK_CONTAINER(vs->statusbox), vs->statusbar);
	//gtk_box_pack_start(GTK_BOX(vs->statusbox), vs->statusbar, TRUE, TRUE, 0);

	push_message(vs->statusbar, vs->context_id, "x");
}

void virtualstudio_close(virtualstudio *vs, int destroy)
{
	audiojam_close(&(vs->aj));
	audioout_close(&(vs->ao));
	if (destroy)
		gtk_widget_destroy(vs->vbox);
}
