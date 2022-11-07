/*
 * VSJam.c
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

#include "VSJam.h"

// Audio Out

// thread
gboolean audioout_led(gpointer data)
{
	audioout *ao = (audioout *)data;

	if (ao->mx.mixerstatus == MX_RUNNING)
		gtk_image_set_from_file(GTK_IMAGE(ao->led), "./images/green.png");
	else
		gtk_image_set_from_file(GTK_IMAGE(ao->led), "./images/red.png");

	return FALSE;
}

static gpointer audioout_thread0(gpointer args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	audioout *ao = (audioout *)args;
	audioencoder *aen = (audioencoder *)&(ao->aen);
	mxthreadparameters *tp = (mxthreadparameters *)&(ao->tp);

	speaker *spk = &(tp->spk);

	init_audiomixer(ao->mixerChannels, MX_BLOCKING, ao->format, ao->rate, ao->frames, ao->channels, &(ao->mx));

	pthread_mutex_lock(&(ao->mxmutex));
	ao->mxready = 1;
	pthread_cond_signal(&(ao->mxinitcond));
	pthread_mutex_unlock(&(ao->mxmutex));

	init_spk(spk, tp->device, ao->format, ao->rate, ao->channels);

	int err;
	if ((err=init_audio_hw_spk(spk)))
	{
		printf("Init spk error %d\n", err);
	}
	else
	{
		gdk_threads_add_idle(audioout_led, ao);
		while (readfrommixer(&(ao->mx)) != MX_STOPPED)
		{
			// process mixed stereo frames here

			write_spk(spk, ao->mx.outbuffer, ao->mx.outbufferframes);

			encoder_add_buffer(aen, ao->mx.outbuffer, ao->mx.outbuffersize); // is a blocking call !
		}
		close_audio_hw_spk(spk);
	}
	close_spk(spk);
	close_audiomixer(&(ao->mx));

//printf("exiting %s\n", ao->name);
	ao->tp.retval = 0;
	pthread_exit(&(ao->tp.retval));
}

void audioout_create_thread(audioout *ao, char *device, unsigned int frames)
{
	int err, ret;

	ao->tp.ao = ao;
	strcpy(ao->tp.device, device);
	ao->tp.frames = frames;

	if ((ret=pthread_mutex_init(&(ao->mxmutex), NULL))!=0)
		printf("audio out mxmutex init failed, %d\n", ret);

	if ((ret=pthread_cond_init(&(ao->mxinitcond), NULL))!=0 )
		printf("audio aout mxinitcond init failed, %d\n", ret);

	ao->mxready = 0;

	err = pthread_create(&(ao->tp.tid), NULL, &audioout_thread0, (void*)ao);
	if (err)
	{}
//printf("thread %s\n", ao->name);
	CPU_ZERO(&(ao->tp.cpu));
	CPU_SET(3, &(ao->tp.cpu));
	if ((err=pthread_setaffinity_np(ao->tp.tid, sizeof(cpu_set_t), &(ao->tp.cpu))))
	{
		//printf("pthread_setaffinity_np error %d\n", err);
	}

	pthread_mutex_lock(&(ao->mxmutex));
	while (!ao->mxready)
	{
		pthread_cond_wait(&(ao->mxinitcond), &(ao->mxmutex));
	}
	pthread_mutex_unlock(&(ao->mxmutex));

	pthread_cond_destroy(&(ao->mxinitcond));
	pthread_mutex_destroy(&(ao->mxmutex));
}

static gpointer audioout_thread_pulseaudio0(gpointer args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	audioout *ao = (audioout *)args;
	audioencoder *aen = (audioencoder *)&(ao->aen);
	//mxthreadparameters *tp = (mxthreadparameters *)&(ao->tp);
	paplayer pa;

	//speaker *spk = &(tp->spk);

	init_audiomixer(ao->mixerChannels, MX_BLOCKING, ao->format, ao->rate, ao->frames, ao->channels, &(ao->mx));

	pthread_mutex_lock(&(ao->mxmutex));
	ao->mxready = 1;
	pthread_cond_signal(&(ao->mxinitcond));
	pthread_mutex_unlock(&(ao->mxmutex));

	//init_spk(spk, tp->device, ao->format, ao->rate, ao->channels);
	init_paplayer(&pa, ao->format, ao->rate, ao->channels);
	//int err;
	//if ((err=init_audio_hw_spk(spk)))
	//{
	//	printf("Init spk error %d\n", err);
	//}
	//else
	//{
		gdk_threads_add_idle(audioout_led, ao);
		while (readfrommixer(&(ao->mx)) != MX_STOPPED)
		{
			// process mixed stereo frames here

			//write_spk(spk, ao->mx.outbuffer, ao->mx.outbufferframes);
			paplayer_add(&pa, ao->mx.outbuffer, ao->mx.outbuffersize);

			encoder_add_buffer(aen, ao->mx.outbuffer, ao->mx.outbuffersize); // is a blocking call !
		}
		//close_audio_hw_spk(spk);
	//}
	//close_spk(spk);
	close_paplayer(&pa);

	close_audiomixer(&(ao->mx));

//printf("exiting %s\n", ao->name);
	ao->tp.retval = 0;
	pthread_exit(&(ao->tp.retval));
}

void audioout_create_thread_pulseaudio(audioout *ao, char *device, unsigned int frames)
{
	int err, ret;

	ao->tp.ao = ao;
	strcpy(ao->tp.device, device);
	ao->tp.frames = frames;

	if ((ret=pthread_mutex_init(&(ao->mxmutex), NULL))!=0)
		printf("audio out mxmutex init failed, %d\n", ret);

	if ((ret=pthread_cond_init(&(ao->mxinitcond), NULL))!=0 )
		printf("audio aout mxinitcond init failed, %d\n", ret);

	ao->mxready = 0;

	err = pthread_create(&(ao->tp.tid), NULL, &audioout_thread_pulseaudio0, (void*)ao);
	if (err)
	{}
//printf("thread %s\n", ao->name);
	CPU_ZERO(&(ao->tp.cpu));
	CPU_SET(3, &(ao->tp.cpu));
	if ((err=pthread_setaffinity_np(ao->tp.tid, sizeof(cpu_set_t), &(ao->tp.cpu))))
	{
		//printf("pthread_setaffinity_np error %d\n", err);
	}

	pthread_mutex_lock(&(ao->mxmutex));
	while (!ao->mxready)
	{
		pthread_cond_wait(&(ao->mxinitcond), &(ao->mxmutex));
	}
	pthread_mutex_unlock(&(ao->mxmutex));

	pthread_cond_destroy(&(ao->mxinitcond));
	pthread_mutex_destroy(&(ao->mxmutex));
}

void audioout_terminate_thread(audioout *ao)
{
	int i;

	signalstop_audiomixer(&(ao->mx));

	gdk_threads_add_idle(audioout_led, ao);

	if (ao->tp.tid)
	{
		if ((i=pthread_join(ao->tp.tid, NULL)))
			printf("pthread_join error, %s, %d\n", ao->name, i);
		ao->tp.tid = 0;
	}
}

void audioout_terminate_thread_pulseaudio(audioout *ao)
{
	int i;

	signalstop_audiomixer(&(ao->mx));

	gdk_threads_add_idle(audioout_led, ao);

	if (ao->tp.tid)
	{
		if ((i=pthread_join(ao->tp.tid, NULL)))
			printf("pthread_join error, %s, %d\n", ao->name, i);
		ao->tp.tid = 0;
	}
}

odevicetype get_odevicetype(char *device)
{
	if (!strcmp(device, "pulseaudio"))
	{
		return pulseaudio;
	}
	else
	{
		return ohardwaredevice;
	}
}

static void outputdevicescombo_changed(GtkWidget *combo, gpointer data)
{
	audioout *ao = (audioout *)data;
	audiojam *aj = ao->aj;
	audioeffectchain *aec;
	int i;

	for(i=0;i<aj->maxchains;i++)
	{
		aec = &(aj->aec[i]);
		if (aec->id)
		{
			if (get_devicetype(aec->tp.device)==hardwaredevice)
				audioeffectchain_terminate_thread(aec);
			else
				audioeffectchain_terminate_thread_ffmpeg(aec);

			gdk_threads_add_idle(audioeffectchain_led, aec);
		}
	}

	if (get_odevicetype(ao->tp.device)==ohardwaredevice)
		audioout_terminate_thread(ao);
	else
		audioout_terminate_thread_pulseaudio(ao);

	gchar *device;
	g_object_get((gpointer)ao->outputdevices, "active-id", &device, NULL);
	if (get_odevicetype(device)==ohardwaredevice)
		audioout_create_thread(ao, device, ao->frames);
	else
		audioout_create_thread_pulseaudio(ao, device, ao->frames);
	g_free(device);

	for(i=0;i<aj->maxchains;i++)
	{
		aec = &(aj->aec[i]);
		if (aec->id)
		{
			//gchar *device;
			//g_object_get((gpointer)aec->inputdevices, "active-id", &device, NULL);
			//audioeffectchain_create_thread(aec, device, aec->frames, aec->channelbuffers, aec->mx);
			//g_free(device);

			g_object_get((gpointer)aec->inputdevices, "active-id", &device, NULL);
			if (get_devicetype(device)==hardwaredevice)
				audioeffectchain_create_thread(aec, device, aec->frames, aec->channelbuffers, aec->mx);
			else
				audioeffectchain_create_thread_ffmpeg(aec, device, aec->frames, aec->channelbuffers, aec->mx);
			g_free(device);
		}
	}
}

static void frames_changed(GtkWidget *widget, gpointer data)
{
	int frames = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	audioout *ao = (audioout *)data;
	audiojam *aj = ao->aj;

	audiojam_close(aj);
	gtk_widget_destroy(aj->toolbarhbox);
	gtk_widget_destroy(aj->frame);
	audioout_terminate_thread(ao);

	ao->frames = frames;

	audioout_create_thread(ao, ao->tp.device, ao->frames);
	audiojam_init(aj, aj->maxchains, aj->maxeffects, aj->format, aj->rate, aj->channels, ao->frames, aj->container, aj->dbpath, &(ao->mx), aj->window);
	gtk_widget_show_all(aj->toolbarhbox);
	gtk_widget_show_all(aj->frame);
}

gboolean setrecordingswitchstate(gpointer data)
{
	audioout *ao = (audioout *)data;
	audioencoder *aen = (audioencoder *)&(ao->aen);
	recordingstate rs;

	rs = encoder_getstate(aen);
	gtk_switch_set_active(GTK_SWITCH(ao->recordswitch), (rs==RS_RECORDING));

	return FALSE;
}

int get_format_from_combobox(GtkWidget *combo)
{
	gchar *strval;
	int value;

	g_object_get((gpointer)combo, "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	value = atoi((const char *)strval);
	g_free(strval);

	return(value);
}

gboolean recordswitch_state_set(GtkSwitch *widget, gboolean state, gpointer user_data)
{
	audioout *ao = (audioout *)user_data;
	audioencoder *aen = (audioencoder *)&(ao->aen);

	GtkWidget *dialog;
	GtkFileFilter *filter;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	char *filterstr;
	enum AVCodecID codecid;
	enum AVSampleFormat avformat;

//printf("%s\n", (state?"on":"off"));
	if (state)
	{
		dialog = gtk_file_chooser_dialog_new("Save File", GTK_WINDOW(ao->window), action, "Cancel", GTK_RESPONSE_CANCEL, "Save", GTK_RESPONSE_ACCEPT, NULL);
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);

		filter = gtk_file_filter_new();
		switch (get_format_from_combobox(ao->recordformats))
		{
			case 0:
				filterstr = "*.mp3";
				codecid = AV_CODEC_ID_MP3;
				avformat = AV_SAMPLE_FMT_S16P;
				break;
			case 1:
				filterstr = "*.aac";
				codecid = AV_CODEC_ID_AAC;
				avformat = AV_SAMPLE_FMT_FLTP;
				break;
			case 2:
				filterstr = "*.flac";
				codecid = AV_CODEC_ID_FLAC;
				avformat = AV_SAMPLE_FMT_S16;
				break;
			default:
				filterstr = "*.mp3";
				codecid = AV_CODEC_ID_MP3;
				avformat = AV_SAMPLE_FMT_S16P;
				break;
		}

		gtk_file_filter_add_pattern(filter, filterstr);
		gtk_file_chooser_add_filter(chooser, filter);
		gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			char *filename = gtk_file_chooser_get_filename(chooser);
			ao->recordedfilename = malloc(strlen(filename)+1);
			strcpy(ao->recordedfilename, filename);
			g_free(filename);

			int err = start_encoder(&(ao->aen), ao->recordedfilename, codecid, avformat, ao->format, ao->rate, ao->channels, 96000);
			if (!err)
			{
			}
			else
			{
				gdk_threads_add_idle(setrecordingswitchstate, (void*)ao);
			}
		}
		else
		{
			gdk_threads_add_idle(setrecordingswitchstate, (void*)ao);
		}
		gtk_widget_destroy(dialog);
	}
	else
	{
		stop_encoder(aen);
		free(ao->recordedfilename);
	}
	return TRUE;
}

void recordformats_changed(GtkWidget *combo, gpointer data)
{
//printf("combo %d\n", get_format_from_combobox(combo));
}

void audioout_init(audioout *ao, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, int mixerChannels, audiojam *aj, GtkWidget *container, GtkWidget *window)
{
	strcpy(ao->name, "Audio Mixer");
	ao->format = format;
	ao->rate = rate;
	ao->channels = channels;
	ao->frames = frames;
	ao->mixerChannels = mixerChannels;
	ao->aj = aj;
	ao->tp.tid = 0;

	ao->window = window;
	ao->container = container;

	init_encoder(&(ao->aen));

// frame
	ao->outputframe = gtk_frame_new("Audio Mixer");
	//gtk_container_add(GTK_CONTAINER(ao->container), ao->outputframe);
	gtk_box_pack_start(GTK_BOX(ao->container), ao->outputframe, TRUE, TRUE, 0);

// horizontal box
	ao->outputhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(ao->outputframe), ao->outputhbox);
	//gtk_box_pack_start(GTK_BOX(ao->outputframe), ao->outputhbox, TRUE, TRUE, 0);

// output devices vbox
	ao->outputdevicesvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(ao->outputhbox), ao->outputdevicesvbox);
	//gtk_box_pack_start(GTK_BOX(ao->outputhbox), ao->outputdevicesvbox, FALSE, TRUE, 0);

// output devices combo
	ao->outputdevices = gtk_combo_box_text_new();
	populate_output_devices_list(ao->outputdevices);
	g_signal_connect(GTK_COMBO_BOX(ao->outputdevices), "changed", G_CALLBACK(outputdevicescombo_changed), ao);
	gtk_container_add(GTK_CONTAINER(ao->outputdevicesvbox), ao->outputdevices);
	//gtk_box_pack_start(GTK_BOX(ao->outputdevicesvbox), ao->outputdevices, TRUE, TRUE, 0);

// output device led vbox
	ao->ledvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(ao->outputhbox), ao->ledvbox);
	//gtk_box_pack_start(GTK_BOX(ao->outputhbox), ao->vbox1, TRUE, TRUE, 0);

// output device led
	ao->led = gtk_image_new();
	gtk_image_set_from_file(GTK_IMAGE(ao->led), "./images/red.png");
	gtk_container_add(GTK_CONTAINER(ao->ledvbox), ao->led);

// frames vbox
	ao->framesvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(ao->outputhbox), ao->framesvbox);
	//gtk_box_pack_start(GTK_BOX(ao->outputhbox), ao->vbox1, TRUE, TRUE, 0);

// frames hbox
	ao->frameshbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(ao->framesvbox), ao->frameshbox);
	//gtk_box_pack_start(GTK_BOX(ao->outputhbox), ao->vbox1, TRUE, TRUE, 0);

// frames
	ao->frameslabel = gtk_label_new("Frames");
	gtk_container_add(GTK_CONTAINER(ao->frameshbox), ao->frameslabel);
	ao->spinbutton = gtk_spin_button_new_with_range(32.0, 1024.0 , 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(ao->spinbutton), (float)ao->frames);
	g_signal_connect(GTK_SPIN_BUTTON(ao->spinbutton), "value-changed", G_CALLBACK(frames_changed), ao);
	gtk_container_add(GTK_CONTAINER(ao->frameshbox), ao->spinbutton);

// recod vbox
	ao->recordvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(ao->outputhbox), ao->recordvbox);
	//gtk_box_pack_start(GTK_BOX(ao->outputhbox), ao->vbox1, TRUE, TRUE, 0);

// record hbox
	ao->recordhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(ao->recordvbox), ao->recordhbox);
	//gtk_box_pack_start(GTK_BOX(ao->outputhbox), ao->vbox1, TRUE, TRUE, 0);

// record label
	ao->recordlabel = gtk_label_new("Record");
	gtk_container_add(GTK_CONTAINER(ao->recordhbox), GTK_WIDGET(ao->recordlabel));

// recording formats combo
	ao->recordformats = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ao->recordformats), "0", "mp3");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ao->recordformats), "1", "aac");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(ao->recordformats), "2", "flac");
	g_object_set((gpointer)ao->recordformats, "active-id", "0", NULL);
	g_signal_connect(GTK_COMBO_BOX(ao->recordformats), "changed", G_CALLBACK(recordformats_changed), (void *)ao->recordformats);
	gtk_container_add(GTK_CONTAINER(ao->recordhbox), GTK_WIDGET(ao->recordformats));

// record switch vertical box
	ao->recordswitchvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(ao->outputhbox), ao->recordswitchvbox);
	//gtk_box_pack_start(GTK_BOX(ao->outputhbox), ao->vbox1, TRUE, TRUE, 0);

// record switch
	ao->recordswitch = gtk_switch_new();
	g_signal_connect(GTK_SWITCH(ao->recordswitch), "state-set", G_CALLBACK(recordswitch_state_set), (void *)ao);
	gtk_container_add(GTK_CONTAINER(ao->recordswitchvbox), GTK_WIDGET(ao->recordswitch));
	//gtk_box_pack_start(GTK_BOX(ao->toolbarframehbox), ao->recordswitch, FALSE, FALSE, 0);

	gchar *device;
	g_object_get((gpointer)ao->outputdevices, "active-id", &device, NULL);
	audioout_create_thread(ao, device, ao->frames);
	g_free(device);
}

void audioout_close(audioout *ao)
{
	audioout_terminate_thread(ao);

	close_encoder(&(ao->aen));
}

// Audio Effect Chains

void addchainbutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audiojam *aj = (audiojam*)data;

	const gchar *text = gtk_entry_get_text(GTK_ENTRY(aj->chainnameentry));
	if (strcmp((char *)text, ""))
		audiojam_addchain(aj, (char *)text);
//	g_print("Button clicked\n");
}

void savechainsbutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audiojam *aj = (audiojam*)data;

	for(aj->aeci=0;aj->aeci<aj->maxchains;aj->aeci++)
	{
		if (!aj->aec[aj->aeci].id)
			continue;
		audioeffectchain_save(&(aj->aec[aj->aeci]));
	}
//	g_print("Button clicked\n");
}

int audiojam_selectchains_callback(void *data, int argc, char **argv, char **azColName) 
{
	audiojam *aj = (audiojam*)data;

	audioeffectchain_init(&(aj->aec[aj->aeci]), argv[1], atoi((const char *)argv[0]), aj->mx, aj->hbox, aj->dbpath);
	aj->aeci++;
	return 0;
}

int audiojam_selectinputs_callback(void *data, int argc, char **argv, char **azColName) 
{
	audiojam *aj = (audiojam*)data;
	int i, id = atoi((const char *)argv[0]);

	for(i=0;i<aj->maxchains;i++)
	{
		if (aj->aec[i].id == id)
		{
			g_object_set((gpointer)aj->aec[i].inputdevices, "active-id", argv[1], NULL);
			break;
		}
	}
	return 0;
}

int audiojam_select_audioeffects_callback(void *data, int argc, char **argv, char **azColName) 
{
	int id, effid;
	audiojam *aj = (audiojam*)data;

	id = atoi((const char *)argv[0]);
	if ((effid = audioeffectchain_loadeffect(&(aj->aec[aj->aeci]), id, argv[1]))>=0)
	{}
	else
		printf("audioeffectchain_loadeffect error %d\n", effid);

	return 0;
}

void audiojam_loadfromdb(audiojam *aj)
{
	int i;
	sqlite3 *db;
	char *err_msg = NULL;
	char sql[200];
	int rc;

	if ((rc = sqlite3_open(aj->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		aj->aeci = 0;

		strcpy(sql, "select * from audiochains;");
		if ((rc = sqlite3_exec(db, sql, audiojam_selectchains_callback, (void*)aj, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		strcpy(sql, "select * from chaininputs;");
		if ((rc = sqlite3_exec(db, sql, audiojam_selectinputs_callback, (void*)aj, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

/*
	}
	sqlite3_close(db);


	if ((rc = sqlite3_open(aj->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
*/
		for(i=0;i<aj->maxchains;i++)
		{
			if (aj->aec[i].id)
			{
				sprintf(sql, "select audioeffects.id, audioeffects.path from chaineffects inner join audioeffects on audioeffects.id = chaineffects.effect where chaineffects.chain = %d;", aj->aec[i].id);
				//printf("%s\n", sql);
				aj->aeci = i;
				if ((rc = sqlite3_exec(db, sql, audiojam_select_audioeffects_callback, (void*)aj, &err_msg)) != SQLITE_OK)
				{
					printf("Failed to select data, %s\n", err_msg);
					sqlite3_free(err_msg);
				}
				else
				{
// success
				}
			}
		}
	}
	sqlite3_close(db);
}

void audiojam_init(audiojam *aj, int maxchains, int maxeffects, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, GtkWidget *container, char *dbpath, audiomixer *mx, GtkWidget *window)
{
	int i, j, ret;

	aj->format = format;
	aj->rate = rate;
	aj->channels = channels;
	aj->frames = frames;
	aj->maxchains = maxchains;
	aj->maxeffects = maxeffects;
	strcpy(aj->dbpath, dbpath);
	aj->mx = mx;
	aj->window = window;

	aj->container = container;

// horizontal box
	aj->toolbarhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(aj->container), aj->toolbarhbox);

// toolbarframe
	aj->toolbarframe = gtk_frame_new("Toolbar");
	gtk_container_add(GTK_CONTAINER(aj->toolbarhbox), aj->toolbarframe);
	//gtk_box_pack_start(GTK_BOX(aj->toolbarhbox), aj->toolbarframe, TRUE, TRUE, 0);

// toolbarframe horizontal box
	aj->toolbarframehbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(aj->toolbarframe), aj->toolbarframehbox);
	//gtk_box_pack_start(GTK_BOX(aj->toolbarframe), aj->toolbarframehbox, TRUE, TRUE, 0);

// toolbar
	aj->toolbar = gtk_toolbar_new();
	gtk_container_add(GTK_CONTAINER(aj->toolbarframehbox), aj->toolbar);
	//gtk_box_pack_start(GTK_BOX(aj->toolbarhbox), aj->toolbar, TRUE, TRUE, 0);

	aj->icon_widget = gtk_image_new_from_icon_name("document-save", GTK_ICON_SIZE_BUTTON);
	aj->savechainsbutton = gtk_tool_button_new(aj->icon_widget, "Save Chains");
	gtk_tool_item_set_tooltip_text(aj->savechainsbutton, "Save Chains");
	g_signal_connect(aj->savechainsbutton, "clicked", G_CALLBACK(savechainsbutton_clicked), aj);
	gtk_toolbar_insert(GTK_TOOLBAR(aj->toolbar), aj->savechainsbutton, -1);

	aj->icon_widget = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
	aj->addchainbutton = gtk_tool_button_new(aj->icon_widget, "Add Chain");
	gtk_tool_item_set_tooltip_text(aj->addchainbutton, "Add Chain");
	g_signal_connect(aj->addchainbutton, "clicked", G_CALLBACK(addchainbutton_clicked), aj);
	gtk_toolbar_insert(GTK_TOOLBAR(aj->toolbar), aj->addchainbutton, -1);

	aj->chainnameentry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(aj->toolbarframehbox), GTK_WIDGET(aj->chainnameentry));

// frame
	aj->frame = gtk_frame_new("Effect Chains");
	//gtk_container_add(GTK_CONTAINER(aj->container), aj->frame);
	gtk_box_pack_start(GTK_BOX(aj->container), aj->frame, TRUE, TRUE, 0);

// horizontal box
	aj->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(aj->frame), aj->hbox);

	aj->aec = malloc(sizeof(audioeffectchain) * maxchains);
	for(i=0;i<aj->maxchains;i++)
	{
		aj->aec[i].id = 0;
		aj->aec[i].format = aj->format;
		aj->aec[i].rate = aj->rate;
		aj->aec[i].channels = aj->channels;
		aj->aec[i].frames = aj->frames;
		if ((ret=pthread_mutex_init(&(aj->aec[i].rackmutex), NULL))!=0)
			printf("rack mutex init failed, %d\n", ret);
		strcpy(aj->aec[i].dbpath, aj->dbpath);
		aj->aec[i].tp.tid = 0;
		aj->aec[i].channelbuffers = 2;
		aj->aec[i].tp.status = TH_STOPPED;

		aj->aec[i].effects = maxeffects;
		aj->aec[i].ae = malloc(sizeof(audioeffect) * aj->aec[i].effects);
		for(j=0;j<aj->aec[i].effects;j++)
		{
			aj->aec[i].ae[j].parent = &(aj->aec[i]);
			aj->aec[i].ae[j].handle = NULL;
			aj->aec[i].ae[j].index = j;
			aj->aec[i].ae[j].format = aj->aec[i].format;
			aj->aec[i].ae[j].rate = aj->aec[i].rate;
			aj->aec[i].ae[j].channels = aj->aec[i].channels;
			if ((ret=pthread_mutex_init(&(aj->aec[i].ae[j].effectmutex), NULL))!=0)
				printf("effect mutex init failed, %d\n", ret);
		}
		aj->aec[i].aeorder = malloc(sizeof(int) * aj->aec[i].effects);
		for(j=0;j<aj->aec[i].effects;j++)
		{
			aj->aec[i].aeorder[j] = -1;
		}
	}
	audiojam_loadfromdb(aj);
}

void audiojam_addchain(audiojam *aj, char *name)
{
	int i;

	for(i=0;i<aj->maxchains;i++)
		if (!aj->aec[i].id) break;

	if (i<aj->maxchains)
	{
		audioeffectchain_init(&(aj->aec[i]), name, i+1, aj->mx, aj->hbox, aj->dbpath);
		gtk_widget_show_all(aj->toolbarhbox);
		gtk_widget_show_all(aj->frame);
	}
	else
		printf("audio chains full, max %d\n", aj->maxchains);
}

void audiojam_close(audiojam *aj)
{
	int i, c;

	for(c=0;c<aj->maxchains;c++)
	{
		audioeffectchain_close(&(aj->aec[c]));
		for(i=0;i<aj->aec[c].effects;i++)
		{
			audioeffectchain_unloadeffect(&(aj->aec[c]), i);
			pthread_mutex_destroy(&(aj->aec[c].ae[i].effectmutex));
		}
		free(aj->aec[c].ae);
		free(aj->aec[c].aeorder);
		pthread_mutex_destroy(&(aj->aec[c].rackmutex));
	}
	free(aj->aec);
}
