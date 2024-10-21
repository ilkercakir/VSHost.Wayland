/*
 * VSHost.c
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

// compile with gcc -Wall -c "%f" -std=c99 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -g -ftree-vectorize -pipe -Wno-psabi $(pkg-config --cflags gtk+-3.0) -Wno-deprecated-declarations
// link with gcc -Wall -o "%e" "%f" VStudio.o VSJam.o VSEffect.o AudioDev.o AudioMic.o AudioMixer.o AudioSpk.o -fPIC -D_POSIX_C_SOURCE=199309L $(pkg-config --cflags gtk+-3.0) -Wl,--whole-archive -lpthread -lrt -ldl -lm -Wl,--no-whole-archive -rdynamic $(pkg-config --libs gtk+-3.0) $(pkg-config --libs libavcodec libavformat libavutil libswscale libswresample) -lasound $(pkg-config --libs sqlite3)

#define _GNU_SOURCE

#include <locale.h>
#include "VStudio.h"

virtualstudio vs;

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{

	return FALSE; // return FALSE to emit destroy signal
}

void audiojam_finalize(audiojam *aj)
{
	int c;

	for(c=0;c<aj->maxchains;c++)
	{
		{
			playlistparams *plparams = &(aj->aec[c].tp.plparams);
			pthread_mutex_destroy(&(plparams->threadmutex));
			pthread_cond_destroy(&(plparams->threadcond));
		}
	}
	free(aj->aec);
}

static void destroy(GtkWidget *widget, gpointer data)
{
	virtualstudio_close(&vs, FALSE);

// Flush callbacks
	while(g_main_context_pending(NULL))
		g_main_context_iteration(NULL, FALSE);

	audiojam *aj = &(vs.aj);
	audiojam_finalize(aj);

	gtk_main_quit();
}

static void realize_cb(GtkWidget *widget, gpointer data)
{

}

void setup_default_icon(char *filename)
{
	GdkPixbuf *pixbuf;
	GError *err;

	err = NULL;
	pixbuf = gdk_pixbuf_new_from_file(filename, &err);

	if (pixbuf)
	{
		GList *list;      

		list = NULL;
		list = g_list_append(list, pixbuf);
		gtk_window_set_default_icon_list(list);
		g_list_free(list);
		g_object_unref(pixbuf);
    	}
}

int main(int argc, char **argv)
{
	GtkWidget *window;

	setlocale(LC_ALL, "tr_TR.UTF-8");

	setup_default_icon("./images/VSHost.png");

	gtk_init(&argc, &argv);

	/* create a new window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER (window), 2);
	//gtk_widget_set_size_request(window, 100, 100);
	gtk_window_set_title(GTK_WINDOW(window), "VSHost");
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
	/* When the window is given the "delete-event" signal (this is given
	* by the window manager, usually by the "close" option, or on the
	* titlebar), we ask it to call the delete_event () function
	* as defined above. The data passed to the callback
	* function is NULL and is ignored in the callback function. */
	g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), NULL);
	/* Here we connect the "destroy" event to a signal handler.  
	* This event occurs when we call gtk_widget_destroy() on the window,
	* or if we return FALSE in the "delete-event" callback. */
	g_signal_connect(window, "destroy", G_CALLBACK (destroy), NULL);
	g_signal_connect(window, "realize", G_CALLBACK (realize_cb), NULL);

	//virtualstudio_purgeeffects(&vs); db path init?
	//virtualstudio_addeffect(&vs, "/home/pi/Desktop/C/VSHost/Effect1.so");
	virtualstudio_init(&vs, 3, 5, SND_PCM_FORMAT_S16, 48000, 2, 128, window, "/var/sqlite3DATA/mediaplayer.db");
	// check db for stored settings in case maxchains, maxeffects change!

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}
