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

#include "VSEffect.h"
#include "AudioDev.h"

// Audio effect
gboolean widget_state_set(GtkWidget *togglebutton, gboolean state, gpointer data)
{
	audioeffectparameter *p = (audioeffectparameter*)data;
	audioeffect *ae = (audioeffect*)(p->parent);

	audioeffect_setparameter(ae, p->index, (float)state);
	return(TRUE);
}

void widget_value_changed(GtkWidget *widget, gpointer data)
{
	audioeffectparameter *p = (audioeffectparameter*)data;
	audioeffect *ae = (audioeffect*)(p->parent);

	float value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
	if (audioeffect_getparameter(ae, p->index)!=value)
		audioeffect_setparameter(ae, p->index, value);
}

static void widget_toggled(GtkWidget *togglebutton, gpointer data)
{
	audioeffectparameter *p = (audioeffectparameter*)data;
	audioeffect *ae = (audioeffect*)(p->parent);

	audioeffect_setparameter(ae, p->index, (float)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)));
}

void widget_scale_value_changed(GtkWidget *widget, gpointer data)
{
	audioeffectparameter *p = (audioeffectparameter*)data;
	audioeffect *ae = (audioeffect*)(p->parent);

	float c = p->minval + p->maxval;
	audioeffect_setparameter(ae, p->index, c - gtk_range_get_value(GTK_RANGE(widget)));
//printf("parameter %d value %2.2f\n", p->index, c - gtk_range_get_value(GTK_RANGE(widget)));
}

gchar* format_value(GtkScale *scale, gdouble value, gpointer data)
{
	audioeffectparameter *p = (audioeffectparameter*)data;
	//audioeffect *ae = (audioeffect*)(p->parent);

	float c = p->minval + p->maxval;
	return g_strdup_printf("%0.1f", c - value);
}

void combo_changed(GtkWidget *combo, gpointer data)
{
	audioeffectparameter *p = (audioeffectparameter*)data;
	audioeffect *ae = (audioeffect*)(p->parent);
	gchar *strval;
	float value;

	g_object_get((gpointer)combo, "active-id", &strval, NULL);
	//printf("Selected id %s\n", strval);
	value = atof((const char *)strval);
	g_free(strval);

	audioeffect_setparameter(ae, p->index, value);
}

void combo_readfromfile(GtkWidget *combo, char *path)
{
	char s[10];
	char *line = NULL;
	size_t len = 0;
	FILE *f = fopen(path, "r");
	if (f)
	{
		while(getline(&line, &len, f) > 0)
		{
			//printf("%s", line);
			char *p1 = line;
			char *q;
			if ((q = strstr(p1, ";")))
			{
				q[0] = '\0';
				//printf("id = %s\n", p1);
				float val = atof((const char *)p1);
				sprintf(s, "%2.2f", val);
				gchar *gc = g_strnfill(10, '\0');
				g_stpcpy(gc, (const char *)s);
				char *p2 = q + 1;
				if ((q = strstr(p2, "\n"))) 
					q[0] = '\0';
				//printf("val = %s\n", p2);
				gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), gc, p2);
				g_free(gc);
			}
			free(line); line = NULL; len = 0;
		}
		fclose(f);
	}
	else
	 printf("failed to read combo values from %s\n", path);
}

void removeeffectbutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audioeffect *ae = (audioeffect *)data;
	audioeffectchain *aec = ae->parent;

	pthread_mutex_lock(&(aec->rackmutex));

	pthread_mutex_lock(&(ae->effectmutex));
	if (ae->handle)
	{
		(ae->aef_close)(ae);
		dlclose(ae->handle);
		ae->handle = NULL;
		audioeffectchain_unorder(aec, ae->index);
	}
	pthread_mutex_unlock(&(ae->effectmutex));

	pthread_mutex_unlock(&(aec->rackmutex));

	gtk_widget_destroy(ae->frame);
}

void reparent_effects(audioeffectchain *aec)
{
	int i, j;

	for(i=0;i<aec->effects;i++)
	{
		j = aec->aeorder[i];
		if (j == -1) break;
		g_object_ref(aec->ae[i].frame);
		gtk_container_remove(GTK_CONTAINER(aec->ae[i].container), aec->ae[i].frame);
	}
	for(i=0;i<aec->effects;i++)
	{
		j = aec->aeorder[i];
		if (j == -1) break;
		gtk_box_pack_start(GTK_BOX(aec->ae[j].container), aec->ae[j].frame, TRUE, TRUE, 0);
		//gtk_container_add(GTK_CONTAINER(aec->ae[i].container), aec->ae[i].frame);
		g_object_unref(aec->ae[j].frame);
	}
}

void moveeffectupbutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audioeffect *ae = (audioeffect *)data;
	audioeffectchain *aec = (audioeffectchain *)ae->parent;
	int i, j;

	for(i=0;i<aec->effects;i++)
		if (aec->aeorder[i] == ae->index) break;

	if (i)
	{
		pthread_mutex_lock(&(aec->rackmutex));
		j = aec->aeorder[i-1];
		aec->aeorder[i-1] = aec->aeorder[i];
		aec->aeorder[i] = j;
		pthread_mutex_unlock(&(aec->rackmutex));

		reparent_effects(aec);
	}
}

void moveeffectdownbutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audioeffect *ae = (audioeffect *)data;
	audioeffectchain *aec = (audioeffectchain *)ae->parent;
	int i, j;

	for(i=0;i<aec->effects;i++)
		if (aec->aeorder[i] == ae->index) break;

	if (i<aec->effects-1)
	{
		if (aec->aeorder[i+1] != -1)
		{
			pthread_mutex_lock(&(aec->rackmutex));
			j = aec->aeorder[i+1];
			aec->aeorder[i+1] = aec->aeorder[i];
			aec->aeorder[i] = j;
			pthread_mutex_unlock(&(aec->rackmutex));

			reparent_effects(aec);
		}
	}
}

void audioeffect_init(audioeffect *ae, int id)
{
	int i;
	GtkWidget *w;
	char s[10];

	pthread_mutex_lock(&(ae->effectmutex));
	ae->id = id;
	(ae->aef_init)(ae);
	pthread_mutex_unlock(&(ae->effectmutex));

// frame
	ae->frame = gtk_frame_new(ae->name);
	//gtk_container_add(GTK_CONTAINER(ae->container), ae->frame);
	gtk_box_pack_start(GTK_BOX(ae->container), ae->frame, TRUE, TRUE, 0);

// horizontal box
	ae->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(ae->frame), ae->hbox);
	//gtk_box_pack_start(GTK_BOX(ae->frame), ae->hbox, TRUE, TRUE, 0);

	for(i=0;i<ae->parameters;i++)
	{
		switch (ae->parameter[i].ptype)
		{
			case pt_switch:
				ae->parameter[i].vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
				gtk_container_add(GTK_CONTAINER(ae->hbox), ae->parameter[i].vbox);
				//gtk_box_pack_start(GTK_BOX(ae->hbox), ae->parameter[i].vbox, TRUE, TRUE, 0);
				break;
			default:
				ae->parameter[i].vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
				//gtk_container_add(GTK_CONTAINER(ae->hbox), ae->parameter[i].vbox);
				gtk_box_pack_start(GTK_BOX(ae->hbox), ae->parameter[i].vbox, TRUE, TRUE, 0);
				break;			
		}
		
		switch (ae->parameter[i].ptype)
		{
			case pt_none:
				break;
			case pt_switch:
				ae->parameter[i].swbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
				gtk_container_add(GTK_CONTAINER(ae->parameter[i].vbox), ae->parameter[i].swbox);
				
				w = ae->parameter[i].pwidget = gtk_switch_new();
				gtk_switch_set_active(GTK_SWITCH(w), ae->parameter[i].value);
				g_signal_connect(GTK_SWITCH(w), "state-set", G_CALLBACK(widget_state_set), &(ae->parameter[i]));
				gtk_container_add(GTK_CONTAINER(ae->parameter[i].swbox), w);
				//gtk_box_pack_start(GTK_BOX(ae->parameter[i].swbox), w, TRUE, TRUE, 0);
				break;
			case pt_scale:
				ae->parameter[i].adj = gtk_adjustment_new(ae->parameter[i].minval + ae->parameter[i].maxval - ae->parameter[i].value, ae->parameter[i].minval, ae->parameter[i].maxval, 0.1, 1, 0);
				w = ae->parameter[i].pwidget = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(ae->parameter[i].adj));
				//gtk_widget_set_size_request(w, 20, 100);
				gtk_scale_set_digits(GTK_SCALE(w), 1);
				g_signal_connect(w, "value-changed", G_CALLBACK(widget_scale_value_changed), &(ae->parameter[i]));
				g_signal_connect(w, "format-value", G_CALLBACK(format_value), &(ae->parameter[i]));
				//gtk_container_add(GTK_CONTAINER(ae->parameter[i].vbox), w);
				gtk_box_pack_start(GTK_BOX(ae->parameter[i].vbox), w, TRUE, TRUE, 0);
				break;
			case pt_check:
				w = ae->parameter[i].pwidget = gtk_check_button_new_with_label(ae->parameter[i].name);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), ae->parameter[i].value);
				g_signal_connect(GTK_TOGGLE_BUTTON(w), "toggled", G_CALLBACK(widget_toggled), &(ae->parameter[i]));
				gtk_container_add(GTK_CONTAINER(ae->parameter[i].vbox), w);
				//gtk_box_pack_start(GTK_BOX(ae->parameter[i].vbox), w, TRUE, TRUE, 0);
				break;
			case pt_spin:
				w = ae->parameter[i].pwidget = gtk_spin_button_new_with_range(ae->parameter[i].minval, ae->parameter[i].maxval , ae->parameter[i].step);
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), ae->parameter[i].value);
				g_signal_connect(GTK_SPIN_BUTTON(w), "value-changed", G_CALLBACK(widget_value_changed), &(ae->parameter[i]));
				gtk_container_add(GTK_CONTAINER(ae->parameter[i].vbox), w);
				//gtk_box_pack_start(GTK_BOX(ae->parameter[i].vbox), w, TRUE, TRUE, 0);
				break;
			case pt_combo:
				w = ae->parameter[i].pwidget = gtk_combo_box_text_new();
				combo_readfromfile(w, ae->parameter[i].confpath);
				sprintf(s, "%2.2f", ae->parameter[i].value);
				g_object_set((gpointer)w, "active-id", s, NULL);
				g_signal_connect(GTK_COMBO_BOX(w), "changed", G_CALLBACK(combo_changed), &(ae->parameter[i]));
				gtk_container_add(GTK_CONTAINER(ae->parameter[i].vbox), w);
				//gtk_box_pack_start(GTK_BOX(ae->parameter[i].vbox), w, TRUE, TRUE, 0);
				break;
		}

		ae->parameter[i].label = gtk_label_new(ae->parameter[i].name);
		//gtk_widget_set_size_request(ae->parameter[i].label, 100, 30);
		gtk_container_add(GTK_CONTAINER(ae->parameter[i].vbox), ae->parameter[i].label);
	}

	ae->vtoolbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	//gtk_container_add(GTK_CONTAINER(ae->hbox), ae->vtoolbox);
	gtk_box_pack_start(GTK_BOX(ae->hbox), ae->vtoolbox, TRUE, TRUE, 0);

	ae->toolbar = gtk_toolbar_new();
	gtk_container_add(GTK_CONTAINER(ae->vtoolbox), ae->toolbar);
	g_object_set(G_OBJECT(ae->toolbar), "orientation", GTK_ORIENTATION_VERTICAL, NULL);
	g_object_set(G_OBJECT(ae->toolbar), "toolbar-style", GTK_TOOLBAR_ICONS, NULL);

	ae->icon_widget = gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON);
	ae->removeeffectbutton = gtk_tool_button_new(ae->icon_widget, "Remove Effect");
	gtk_tool_item_set_tooltip_text(ae->removeeffectbutton, "Remove Effect");
	g_signal_connect(ae->removeeffectbutton, "clicked", G_CALLBACK(removeeffectbutton_clicked), ae);
	gtk_toolbar_insert(GTK_TOOLBAR(ae->toolbar), ae->removeeffectbutton, -1);

	ae->icon_up_widget = gtk_image_new_from_icon_name("up", GTK_ICON_SIZE_BUTTON);
	ae->moveeffectupbutton = gtk_tool_button_new(ae->icon_up_widget, "Move Up");
	gtk_tool_item_set_tooltip_text(ae->moveeffectupbutton, "Move Up");
	g_signal_connect(ae->moveeffectupbutton, "clicked", G_CALLBACK(moveeffectupbutton_clicked), ae);
	gtk_toolbar_insert(GTK_TOOLBAR(ae->toolbar), ae->moveeffectupbutton, -1);

	ae->icon_down_widget = gtk_image_new_from_icon_name("down", GTK_ICON_SIZE_BUTTON);
	ae->moveeffectdownbutton = gtk_tool_button_new(ae->icon_down_widget, "Move Down");
	gtk_tool_item_set_tooltip_text(ae->moveeffectdownbutton, "Move Down");
	g_signal_connect(ae->moveeffectdownbutton, "clicked", G_CALLBACK(moveeffectdownbutton_clicked), ae);
	gtk_toolbar_insert(GTK_TOOLBAR(ae->toolbar), ae->moveeffectdownbutton, -1);
}

void audioeffect_allocateparameters(audioeffect *ae, int count)
{
	if (count<=0)
	{
		printf("audioeffect_allocateparameters() count %d not positive\n", count);
		return;
	}
	ae->parameters = count;
	ae->parameter = malloc(sizeof(audioeffectparameter) * ae->parameters);
}

void audioeffect_initparameter(audioeffect *ae, int i, char* name, float minval, float maxval, float value, float step, int resetrequired, parametertype ptype)
{
	if (i<0 || i>=ae->parameters)
	{
		printf("audioeffect_initparameter() index %d out of %d..%d range\n", i, 0, ae->parameters-1);
		return;
	}

	strcpy(ae->parameter[i].name, name);
	ae->parameter[i].minval = minval;
	ae->parameter[i].maxval = maxval;
	ae->parameter[i].ptype = ptype;
	ae->parameter[i].value = value;
	ae->parameter[i].step = step;
	ae->parameter[i].resetrequired = resetrequired;
	ae->parameter[i].parent = (void *)ae;
	ae->parameter[i].index = i;

	char *p, *q, s[4];
	int len;
	p = ae->sopath;
	if ((q = strstr(p, ".so")))
	{
		len = q - p + 1;
		strncpy(ae->parameter[i].confpath, ae->sopath, len);
	}
	else
	{
		len = 0;
	}
	ae->parameter[i].confpath[len] = '\0';

	if (q)
	{
		sprintf(s, "%d", i);
		strcat(ae->parameter[i].confpath, s);
		strcat(ae->parameter[i].confpath, ".conf");
	}
//printf("conf %d = %s\n", i, ae->parameter[i].confpath);
}

void audioeffect_setparameter(audioeffect *ae, int i, float value)
{
	pthread_mutex_lock(&(ae->effectmutex));
	(ae->aef_setparameter)(ae, i, value);
	if (ae->parameter[i].resetrequired)
		(ae->aef_reinit)(ae);
	pthread_mutex_unlock(&(ae->effectmutex));
}

float audioeffect_getparameter(audioeffect *ae, int i)
{
	pthread_mutex_lock(&(ae->effectmutex));
	float value = (ae->aef_getparameter)(ae, i);
	pthread_mutex_unlock(&(ae->effectmutex));

	return value;
}

void audioeffect_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize)
{
	pthread_mutex_lock(&(ae->effectmutex));
	if (ae->handle)
		(ae->aef_process)(ae, inbuffer, inbuffersize);
	pthread_mutex_unlock(&(ae->effectmutex));
}

void audioeffect_deallocateparameters(audioeffect *ae)
{
	free(ae->parameter);
}

void audioeffect_close(audioeffect *ae)
{
	pthread_mutex_lock(&(ae->effectmutex));
	(ae->aef_close)(ae);
	pthread_mutex_unlock(&(ae->effectmutex));
}

gboolean audioeffect_setdependentparameter_idle(gpointer data)
{
	audioeffectidle *aei = (audioeffectidle *)data;
	audioeffect *ae = aei->ae;
	int i = aei->i;
	char s[10];

	GtkWidget *w = ae->parameter[i].pwidget;

	switch (ae->parameter[i].ptype)
	{
		case pt_none:
			break;
		case pt_switch:
			gtk_switch_set_active(GTK_SWITCH(w), aei->value);
			break;
		case pt_scale:
			gtk_adjustment_set_value(ae->parameter[i].adj, ae->parameter[i].minval + ae->parameter[i].maxval - aei->value);
			break;
		case pt_check:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), aei->value);
			break;
		case pt_spin:
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), aei->value);
			break;
		case pt_combo:
			sprintf(s, "%2.2f", aei->value);
			g_object_set((gpointer)w, "active-id", s, NULL);
			break;
	}

	free(aei);

	return FALSE;
}

void audioeffect_setdependentparameter(audioeffect *ae, int i, float value)
{
	audioeffectidle *aei = malloc(sizeof(audioeffectidle));

	aei->ae = ae;
	aei->i = i;
	aei->value = value;

	gdk_threads_add_idle(audioeffect_setdependentparameter_idle, aei);
}

// Audio effect chain

// thread
gboolean audioeffectchain_led(gpointer data)
{
	audioeffectchain *aec = (audioeffectchain *)data;

//printf("%s -> %s\n", aec->name, (aec->tp.mic.status == MC_RUNNING?"green":"red"));

	if (get_devicetype(aec->tp.device)==hardwaredevice)
	{
		if (aec->tp.mic.status == MC_RUNNING)
			gtk_image_set_from_file(GTK_IMAGE(aec->led), "./images/green.png");
		else
			gtk_image_set_from_file(GTK_IMAGE(aec->led), "./images/red.png");
	}
	else
	{
		if (aec->tp.vpw.ap.status == CQ_RUNNING)
			gtk_image_set_from_file(GTK_IMAGE(aec->led), "./images/green.png");
		else
			gtk_image_set_from_file(GTK_IMAGE(aec->led), "./images/red.png");		
	}
	return FALSE;
}

gboolean init_videoplayerwidgets_idle(gpointer data)
{
	playlistparams *plp = (playlistparams *)data;

//printf("");
	init_videoplayerwidgets(plp, 640, 360);

	return FALSE;
}

gboolean close_videoplayerwidgets_idle(gpointer data)
{
	playlistparams *plp = (playlistparams *)data;

//printf("");
	close_videoplayerwidgets(plp);

	return FALSE;
}

static gpointer audioeffectchain_thread0(gpointer args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	int err;
	audioeffectchain *aec = (audioeffectchain *)args;
	threadparameters *tp = (threadparameters *)&(aec->tp);

	init_mic(&(tp->mic), tp->device, aec->format, aec->rate, aec->channels, aec->frames);
	connect_audiojack(tp->channelbuffers, &(tp->jack), aec->mx);

	if ((err=init_audio_hw_mic(&(tp->mic))))
	{
		printf("Init mic error %d %s\n", err, aec->name);
	}
	else
	{
		gdk_threads_add_idle(audioeffectchain_led, aec);
		while ((tp->mic.status==MC_RUNNING) && (tp->status==TH_RUNNING))
		{
			if (read_mic(&(tp->mic)))
			{
				// Process input frames here
				audioeffectchain_process(aec, tp->mic.buffer, tp->mic.buffersize);
//printf("mic %s\n", tp->mic.device);
				writetojack(tp->mic.buffer, tp->mic.buffersize, &(tp->jack));
			}
			else
			{
				printf("stopping mic\n");
				tp->mic.status=MC_STOPPED;
			}
		}
		close_audio_hw_mic(&(tp->mic));
	}
	close_audiojack(&(tp->jack));
	close_mic(&(tp->mic));

//printf("exiting %s\n", aec->name);
	aec->tp.retval = 0;
	pthread_exit(&(aec->tp.retval));
}

void audioeffectchain_create_thread(audioeffectchain *aec, char *device, unsigned int frames, int channelbuffers, audiomixer *mx)
{
	int err;

	aec->mx = mx;
	aec->tp.aec = aec;
	strcpy(aec->tp.device, device);
	aec->tp.frames = frames;
	aec->tp.channelbuffers = channelbuffers;
	aec->tp.status = TH_RUNNING;

	err = pthread_create(&(aec->tp.tid), NULL, &audioeffectchain_thread0, (void*)aec);
	if (err)
	{}
//printf("thread %s -> %d tid %d\n", aec->name, 1, aec->tp.tid);

	CPU_ZERO(&(aec->tp.cpu));
	CPU_SET(3, &(aec->tp.cpu));
	if ((err=pthread_setaffinity_np(aec->tp.tid, sizeof(cpu_set_t), &(aec->tp.cpu))))
	{
		//printf("pthread_setaffinity_np error %d\n", err);
	}
}

static gpointer audioeffectchain_thread_ffmpeg0(gpointer args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	audioeffectchain *aec = (audioeffectchain *)args;
	threadparameters *tp = (threadparameters *)&(aec->tp);
	playlistparams *plp = &(tp->plparams);
	vpwidgets *vpw = &(tp->vpw);

	init_playlistparams(plp, vpw, 20, aec->format, aec->rate, aec->channels, aec->frames, 20*1024, 4); // video, frames, cqframes, thread_count
	gdk_threads_add_idle(init_videoplayerwidgets_idle, plp);

	connect_audiojack(tp->channelbuffers, &(tp->jack), aec->mx);

	gdk_threads_add_idle(audioeffectchain_led, aec);
	audiopipe *ap = &(tp->vpw.ap);
	while ((audioCQ_remove(ap)==CQ_RUNNING) && (tp->status==TH_RUNNING))
	{
		// Process input frames here
		audioeffectchain_process(aec, ap->buffer, ap->buffersize);
//printf("%s %d\n", aec->device, ap->buffersize);
		writetojack(ap->buffer, ap->buffersize, &(tp->jack));
	}

	close_audiojack(&(tp->jack));

	gdk_threads_add_idle(close_videoplayerwidgets_idle, plp);
	close_playlistparams(plp);

//printf("exiting %s\n", aec->name);
	aec->tp.retval = 0;
	pthread_exit(&(aec->tp.retval));
}

void audioeffectchain_create_thread_ffmpeg(audioeffectchain *aec, char *device, unsigned int frames, int channelbuffers, audiomixer *mx)
{
	int err;

	aec->mx = mx;
	aec->tp.aec = aec;
	strcpy(aec->tp.device, device);
	aec->tp.frames = frames;
	aec->tp.channelbuffers = channelbuffers;
	aec->tp.status = TH_RUNNING;

	err = pthread_create(&(aec->tp.tid), NULL, &audioeffectchain_thread_ffmpeg0, (void*)aec);
	if (err)
	{}
//printf("thread %s -> %d tid %d\n", aec->name, 1, aec->tp.tid);
	CPU_ZERO(&(aec->tp.cpu));
	CPU_SET(1, &(aec->tp.cpu));
	CPU_SET(2, &(aec->tp.cpu));
	if ((err=pthread_setaffinity_np(aec->tp.tid, sizeof(cpu_set_t), &(aec->tp.cpu))))
	{
		//printf("pthread_setaffinity_np error %d\n", err);
	}
}

void audioeffectchain_terminate_thread(audioeffectchain *aec)
{
	int i;

	signalstop_mic(&(aec->tp.mic));

	if (aec->tp.tid)
	{
		aec->tp.status = TH_STOPPED;
		if ((i=pthread_join(aec->tp.tid, NULL)))
			printf("pthread_join error, %s, %d\n", aec->name, i);

		aec->tp.tid = 0;
	}
}

void audioeffectchain_terminate_thread_ffmpeg(audioeffectchain *aec)
{
	int i;
	audiopipe *ap = &(aec->tp.vpw.ap);

	audioCQ_signalstop(ap);

	if (aec->tp.tid)
	{
		aec->tp.status = TH_STOPPED;
		if ((i=pthread_join(aec->tp.tid, NULL)))
			printf("pthread_join error, %s, %d\n", aec->name, i);

		aec->tp.tid = 0;
	}
}

// effect chain utilities
static gboolean u_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	return FALSE; // return FALSE to emit destroy signal
}

static void u_destroy(GtkWidget *widget, gpointer data)
{
}

static void u_realize_cb(GtkWidget *widget, gpointer data)
{
}

int u_select_callback(void *data, int argc, char **argv, char **azColName) 
{
	audioeffectchainutil *u = (audioeffectchainutil *)data;
//    NotUsed = 0;
//    for (int i = 0; i < argc; i++)
//    {
//     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//     if (!strcmp(azColName[i],"response"))
//     {
//      gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combobox), argv[i], argv[i]);
//     }
//    }
//    printf("\n");
	//gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combobox), argv[0], argv[1]);

	gtk_list_store_append(u->store, &(u->iter));
	gtk_list_store_set(u->store, &(u->iter), COL_ID, atoi(argv[0]), COL_FILEPATH, argv[1], -1);

//g_print("%s, %s\n", argv[0], argv[1]);
	return 0;
}

static GtkTreeModel* u_create_and_fill_model(audioeffectchain *aec)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char *sql = NULL;
	int rc;
	audioeffectchainutil *u = &(aec->util);

	u->store = gtk_list_store_new(NUM_COLS, G_TYPE_UINT, G_TYPE_STRING);

	if ((rc = sqlite3_open(aec->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sql = "SELECT * FROM audioeffects order by id;";
		if ((rc = sqlite3_exec(db, sql, u_select_callback, u, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);

	return GTK_TREE_MODEL(u->store);
}

gboolean u_focus_iter_idle(gpointer data)
{
	GtkTreeModel *model;
	GtkTreePath *tp;
	audioeffectchainutil *u = (audioeffectchainutil *)data;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(u->view));
	tp = gtk_tree_model_get_path(model, &(u->iter));
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(u->view), tp, NULL, FALSE);
	gtk_tree_path_free(tp);

	return(FALSE);
}

gboolean u_search_equal_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer search_data)
{
	gboolean valid;
	gchararray gca;
	audioeffectchainutil *u = (audioeffectchainutil *)search_data;

	//printf("column %d, key %s\n", column, key);
    for(valid=gtk_tree_model_get_iter_first(model, &(u->iter));valid;valid=gtk_tree_model_iter_next(model, &(u->iter)))
	{
		gtk_tree_model_get(model, &(u->iter), COL_FILEPATH, &gca, -1);
		if (strstr((char*)gca, (char*)key))
		{
//printf("%s, %s\n", gca, key);
			gdk_threads_add_idle(u_focus_iter_idle, u);
			break;
		}
		g_free(gca);
    }

	return(FALSE); // found
}

void u_search_destroy(gpointer data)
{
	//g_free(data);
}

void u_detach_model(audioeffectchain *aec)
{
	GtkTreeModel *model;
	audioeffectchainutil *u = &(aec->util);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(u->view));
	gtk_list_store_clear((GtkListStore*)model);

	gtk_tree_view_set_model(GTK_TREE_VIEW(u->view), NULL); /* Detach model from view */
	g_object_unref(model);

	gtk_widget_destroy(u->view);
}

void u_listview_onRowActivated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
	audioeffectchain *aec = (audioeffectchain *)userdata;
	//audioeffectchainutil *u = &(aec->util);

	GtkTreeModel *model;
	GtkTreeIter iter;
	char *sopath = NULL;
	int id;

//g_print("double-clicked\n");
	model = gtk_tree_view_get_model(treeview);
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		//gchar *s = gtk_tree_path_to_string(path);
		//printf("%s\n",s);
		//g_free(s);
		gtk_tree_model_get(model, &iter, COL_FILEPATH, &sopath, COL_ID, &id, -1);
//printf("Double-clicked path %s id %d\n", sopath, id);

		if ((aec->effid = audioeffectchain_loadeffect(aec, id, sopath))>=0)
		{}
		else
			printf("audioeffectchain_loadeffect error %d\n", aec->effid);

		g_free(sopath);

		gtk_widget_show_all(aec->container);
	}
}

void u_create_view_and_model(audioeffectchain *aec)
{
	GtkCellRenderer *renderer;
	GtkTreeModel *model;
	audioeffectchainutil *u = &(aec->util);

	u->view = gtk_tree_view_new();

	// Column 1
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(u->view), -1, "ID", renderer, "text", COL_ID, NULL);

	// Column 2
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(u->view), -1, "File Path", renderer, "text", COL_FILEPATH, NULL);

	model = u_create_and_fill_model(aec);
	gtk_tree_view_set_model(GTK_TREE_VIEW(u->view), model);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(u->view), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(u->view), COL_FILEPATH);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(u->view), u_search_equal_func, u, u_search_destroy);

	g_signal_connect(u->view, "row-activated", (GCallback)u_listview_onRowActivated, aec);
	gtk_container_add(GTK_CONTAINER(u->scrolled_window), u->view);

/* The tree view has acquired its own reference to the model, so we can drop ours. That way the model will
   be freed automatically when the tree view is destroyed */
//g_object_unref(model);
}

int nosharedlibrary(char *filepath)
{
	return
	(
		strcmp(strlastpart(filepath, ".", 1), ".so")
	);
}

int u_select_max_callback(void *data, int argc, char **argv, char **azColName) 
{
	audioeffectchain *aec = (audioeffectchain *)data;

	aec->effid = (argv[0]?atoi((const char *)argv[0]):0);

	return 0;
}

void u_addeffect(audioeffectchain *aec, char *path)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char *sql;
	int rc;
	char ins[256];

	if ((rc = sqlite3_open(aec->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		aec->effid = 0;

		sql = "select max(id) as id from audioeffects;";
		if ((rc = sqlite3_exec(db, sql, u_select_max_callback, aec, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		aec->effid++;
		sprintf(ins, "insert into audioeffects values(%d, '%s');", aec->effid, path);
		if ((rc = sqlite3_exec(db, ins, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

void u_addbutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audioeffectchain *aec = (audioeffectchain *)data;
	audioeffectchainutil *u = &(aec->util);

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GSList *chosenfile;

	GtkWidget *toplevel = gtk_widget_get_toplevel(aec->container);
	if (!GTK_IS_WINDOW(toplevel))
		return;

	dialog = gtk_file_chooser_dialog_new("Add File", GTK_WINDOW(toplevel), action, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_select_multiple(chooser, TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GSList *filelist = gtk_file_chooser_get_filenames(chooser);
		for(chosenfile=filelist;chosenfile;chosenfile=chosenfile->next)
		{
//printf("%s\n", (char*)chosenfile->data);
			if (nosharedlibrary((char*)chosenfile->data))
				continue;
			u_addeffect(aec, (char*)chosenfile->data);

			u_detach_model(aec);
			u_create_view_and_model(aec);
			gtk_widget_show_all(u->window);
		}
	}
	gtk_widget_destroy(dialog);
//    g_print("Button clicked\n");
}

void u_deleteeffect(audioeffectchain *aec, int id)
{
	sqlite3 *db;
	char *err_msg = NULL;
	int rc;
	char del[256];

	if ((rc = sqlite3_open(aec->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");

		sprintf(del, "delete from audioeffects where id = %d;", id);
		if ((rc = sqlite3_exec(db, del, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

void u_deletebutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audioeffectchain *aec = (audioeffectchain *)data;
	audioeffectchainutil *u = &(aec->util);

	GtkTreeModel *model;
	GtkTreeSelection *selection;
	char *sopath;
	int id;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(u->view));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(u->view));

	if (gtk_tree_selection_get_selected(selection, &model, &(u->iter)))
	{
		gtk_tree_model_get(model, &(u->iter), COL_FILEPATH, &sopath, COL_ID, &id, -1);
		u_deleteeffect(aec, id);

		u_detach_model(aec);
		u_create_view_and_model(aec);
		gtk_widget_show_all(u->window);
	}
}

void audioeffectchain_effectlist(audioeffectchain *aec)
{
	audioeffectchainutil *u = &(aec->util);

    /* create a new window */
	u->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(u->window), GTK_WIN_POS_CENTER);
	/* Sets the border width of the window. */
	gtk_container_set_border_width(GTK_CONTAINER(u->window), 2);
	//gtk_widget_set_size_request(u->window, 150, 100);
	gtk_window_set_title(GTK_WINDOW(u->window), "Audio Effects");
	//gtk_window_set_resizable(GTK_WINDOW(u->window), FALSE);
	gtk_window_set_modal(GTK_WINDOW(u->window), TRUE);
	GtkWidget *toplevel = gtk_widget_get_toplevel(aec->container);
	if (!GTK_IS_WINDOW(toplevel))
		return;
	gtk_window_set_transient_for(GTK_WINDOW(u->window), GTK_WINDOW(toplevel));

	/* When the window is given the "delete-event" signal (this is given
     * by the window manager, usually by the "close" option, or on the
     * titlebar), we ask it to call the delete_event () function
     * as defined above. The data passed to the callback
     * function is NULL and is ignored in the callback function. */
	g_signal_connect(u->window, "delete-event", G_CALLBACK(u_delete_event), aec);
    
	/* Here we connect the "destroy" event to a signal handler.  
     * This event occurs when we call gtk_widget_destroy() on the window,
     * or if we return FALSE in the "delete-event" callback. */
	g_signal_connect(u->window, "destroy", G_CALLBACK(u_destroy), aec);

	g_signal_connect(u->window, "realize", G_CALLBACK(u_realize_cb), aec);

// vbox
	u->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(u->window), u->vbox);


// toolbar
	u->toolbar = gtk_toolbar_new();
	gtk_container_add(GTK_CONTAINER(u->vbox), u->toolbar);

	u->icon_widget = gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_BUTTON);
	u->addbutton = gtk_tool_button_new(u->icon_widget, "Add Effect");
	gtk_tool_item_set_tooltip_text(u->addbutton, "Add Effect");
	g_signal_connect(u->addbutton, "clicked", G_CALLBACK(u_addbutton_clicked), aec);
	gtk_toolbar_insert(GTK_TOOLBAR(u->toolbar), u->addbutton, -1);

	u->icon_widget = gtk_image_new_from_icon_name("edit-delete", GTK_ICON_SIZE_BUTTON);
	u->deletebutton = gtk_tool_button_new(u->icon_widget, "Delete Effect");
	gtk_tool_item_set_tooltip_text(u->deletebutton, "Delete Effect");
	g_signal_connect(u->deletebutton, "clicked", G_CALLBACK(u_deletebutton_clicked), aec);
	gtk_toolbar_insert(GTK_TOOLBAR(u->toolbar), u->deletebutton, -1);

// scrolled window
	u->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(u->scrolled_window), 10);
	gtk_widget_set_size_request(u->scrolled_window, 600, 200);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(u->scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	//gtk_widget_set_size_request(u->scrolled_window, w, h);
	//gtk_container_add(GTK_CONTAINER(u->vbox), u->scrolled_window);
	gtk_box_pack_start(GTK_BOX(u->vbox), u->scrolled_window, TRUE, TRUE, 0);

// listview
	u_create_view_and_model(aec);
	
/*
	gtk_drag_dest_set(vpw->listview, GTK_DEST_DEFAULT_ALL, vpw->target_entries, G_N_ELEMENTS(vpw->target_entries), GDK_ACTION_COPY | GDK_ACTION_MOVE );
	g_signal_connect(vpw->listview, "drag_data_received", G_CALLBACK(drag_data_received_da_event), (void*)plp);
	g_signal_connect(vpw->listview, "drag_drop", G_CALLBACK(drag_drop_da_event), NULL);
	g_signal_connect(vpw->listview, "drag_motion", G_CALLBACK(drag_motion_da_event), NULL);
	g_signal_connect(vpw->listview, "drag_leave", G_CALLBACK(drag_leave_da_event), NULL);
*/

	gtk_widget_show_all(u->window);
}

void audioeffectchain_save(audioeffectchain *aec)
{
	sqlite3 *db;
	char *err_msg = NULL;
	int rc;
	char sql[256];

	if ((rc = sqlite3_open(aec->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");

		sprintf(sql, "delete from chaininputs where chain = %d;", aec->id);
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		sprintf(sql, "delete from chaineffects where chain = %d;", aec->id);
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		sprintf(sql, "delete from audiochains where id = %d;", aec->id);
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		sprintf(sql, "insert into audiochains values(%d, '%s');", aec->id, aec->name);
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		int i;
		for(i=0;i<aec->effects;i++)
		{
			if (!aec->ae[i].handle)
				continue;

			sprintf(sql, "insert into chaineffects values(%d, %d);", aec->id, aec->ae[i].id);
			if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
			{
				printf("Failed to insert data, %s\n", err_msg);
				sqlite3_free(err_msg);
			}
			else
			{
// success
			}
		}

		sprintf(sql, "insert into chaininputs values(%d, '%s');", aec->id, aec->tp.device);
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
}

void audioeffectchain_delete(audioeffectchain *aec)
{
	int i;
/*
	sqlite3 *db;
	char *err_msg = NULL;
	int rc;
	char sql[256];

	if ((rc = sqlite3_open(aec->dbpath, &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");

		sprintf(sql, "delete from chaineffects where chain = %d;", aec->id);
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to insert data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}

		sprintf(sql, "delete from audiochains where id = %d;", aec->id);
		if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to delete data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);
*/

		audioeffectchain_close(aec);
		for(i=0;i<aec->effects;i++)
		{
			audioeffectchain_unloadeffect(aec, i);
		}
		
		gtk_widget_destroy(aec->frame);
}

// effect chain

devicetype get_devicetype(char *device)
{
	if (!strcmp(device, "mediafile"))
	{
		return mediafiledevice;
	}
	else
	{
		return hardwaredevice;
	}
}

static void inputdevicescombo_changed(GtkWidget *combo, gpointer data)
{
	audioeffectchain *aec = (audioeffectchain *)data;
	gchar *device;

	if (get_devicetype(aec->tp.device)==hardwaredevice)
		audioeffectchain_terminate_thread(aec);
	else
		audioeffectchain_terminate_thread_ffmpeg(aec);

	gdk_threads_add_idle(audioeffectchain_led, aec);

	g_object_get((gpointer)aec->inputdevices, "active-id", &device, NULL);
	if (get_devicetype(device)==hardwaredevice)
		audioeffectchain_create_thread(aec, device, aec->frames, aec->channelbuffers, aec->mx);
	else
		audioeffectchain_create_thread_ffmpeg(aec, device, aec->frames, aec->channelbuffers, aec->mx);
	g_free(device);
}

void addeffectbutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audioeffectchain *aec = (audioeffectchain*)data;

	audioeffectchain_effectlist(aec);

//    g_print("Button clicked\n");
}

void saveeffectsbutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audioeffectchain *aec = (audioeffectchain*)data;

	audioeffectchain_save(aec);

//    g_print("Button clicked\n");
}

void deletechainbutton_clicked(GtkToolButton *toolbutton, gpointer data)
{
	audioeffectchain *aec = (audioeffectchain*)data;

	audioeffectchain_delete(aec);

//    g_print("Button clicked\n");
}

void audioeffectchain_init(audioeffectchain *aec, char *name, int id, audiomixer *mx, GtkWidget *container, char *dbpath)
{
	pthread_mutex_lock(&(aec->rackmutex));
	strcpy(aec->name, name);
	aec->id = id;
	pthread_mutex_unlock(&(aec->rackmutex));

	aec->container = container;

// frame
	aec->frame = gtk_frame_new(aec->name);
	//gtk_container_add(GTK_CONTAINER(aec->container), aec->frame);
	gtk_box_pack_start(GTK_BOX(aec->container), aec->frame, TRUE, TRUE, 0);

// box
	aec->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(aec->frame), aec->vbox);

// add input devices combo
// frame
	aec->inputframe = gtk_frame_new("Input");
	gtk_container_add(GTK_CONTAINER(aec->vbox), aec->inputframe);
	//gtk_box_pack_start(GTK_BOX(ar->container), ar->inputframe, TRUE, TRUE, 0);

// horizontal box
	aec->inputhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_container_add(GTK_CONTAINER(aec->inputframe), aec->inputhbox);
	//gtk_box_pack_start(GTK_BOX(ae->frame), ae->hbox, TRUE, TRUE, 0);

// combo box
	aec->inputdevices = gtk_combo_box_text_new();
	populate_input_devices_list(aec->inputdevices); // input hardware devices
	g_signal_connect(GTK_COMBO_BOX(aec->inputdevices), "changed", G_CALLBACK(inputdevicescombo_changed), aec);
	gtk_container_add(GTK_CONTAINER(aec->inputhbox), aec->inputdevices);

	aec->led = gtk_image_new();
	gtk_image_set_from_file(GTK_IMAGE(aec->led), "./images/red.png");
	gtk_container_add(GTK_CONTAINER(aec->inputhbox), aec->led);

// toolbar
	aec->toolbar = gtk_toolbar_new();
	gtk_container_add(GTK_CONTAINER(aec->inputhbox), aec->toolbar);
	//gtk_box_pack_start(GTK_BOX(aec->inputhbox), aec->toolbar, TRUE, TRUE, 0);

	aec->icon_widget = gtk_image_new_from_icon_name("document-save", GTK_ICON_SIZE_BUTTON);
	aec->saveeffectsbutton = gtk_tool_button_new(aec->icon_widget, "Save");
	gtk_tool_item_set_tooltip_text(aec->saveeffectsbutton, "Save");
	g_signal_connect(aec->saveeffectsbutton, "clicked", G_CALLBACK(saveeffectsbutton_clicked), aec);
	gtk_toolbar_insert(GTK_TOOLBAR(aec->toolbar), aec->saveeffectsbutton, -1);

	aec->icon_widget = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
	aec->addeffectbutton = gtk_tool_button_new(aec->icon_widget, "Add Effect");
	gtk_tool_item_set_tooltip_text(aec->addeffectbutton, "Add Effect");
	g_signal_connect(aec->addeffectbutton, "clicked", G_CALLBACK(addeffectbutton_clicked), aec);
	gtk_toolbar_insert(GTK_TOOLBAR(aec->toolbar), aec->addeffectbutton, -1);

	aec->icon_widget = gtk_image_new_from_icon_name("edit-delete", GTK_ICON_SIZE_BUTTON);
	aec->deletechainbutton = gtk_tool_button_new(aec->icon_widget, "Delete Chain");
	gtk_tool_item_set_tooltip_text(aec->deletechainbutton, "Delete Chain");
	g_signal_connect(aec->deletechainbutton, "clicked", G_CALLBACK(deletechainbutton_clicked), aec);
	gtk_toolbar_insert(GTK_TOOLBAR(aec->toolbar), aec->deletechainbutton, -1);

// thread
	gchar *device;
	g_object_get((gpointer)aec->inputdevices, "active-id", &device, NULL);
//	audioeffectchain_create_thread(aec, device, aec->frames, aec->channelbuffers, mx);

	if (get_devicetype(device)==hardwaredevice)
		audioeffectchain_create_thread(aec, device, aec->frames, aec->channelbuffers, mx);
	else
		audioeffectchain_create_thread_ffmpeg(aec, device, aec->frames, aec->channelbuffers, mx);

	g_free(device);
}

void audioeffectchain_order(audioeffectchain *aec, int effect)
{
	int j;

	for(j=0;aec->aeorder[j]>-1;j++);
	aec->aeorder[j] = effect;
//printf("order\n");
//for(j=0;j<aec->effects;j++)
//	printf("%d\n", aec->aeorder[j]);
}

void audioeffectchain_unorder(audioeffectchain *aec, int effect)
{
	int j;

	for(j=0;j<aec->effects;j++)
	{
		if (aec->aeorder[j]==effect)
			break;
	}

	for(j++;j<aec->effects;j++)
		aec->aeorder[j-1] = aec->aeorder[j];

	aec->aeorder[aec->effects-1] = -1;
//printf("order\n");
//for(j=0;j<aec->effects;j++)
//	printf("%d\n", aec->aeorder[j]);
}

int audioeffectchain_loadeffect(audioeffectchain *aec, int id, char *path)
{
	char *error;
	int i;

	pthread_mutex_lock(&(aec->rackmutex));
	for(i=0;i<aec->effects;i++)
	{
		if (!aec->ae[i].handle)
			break;
	}
	if (i<aec->effects) // empty slot found
	{
		audioeffectchain_order(aec, i);

		strcpy(aec->ae[i].sopath, path);
		aec->ae[i].container = aec->vbox;

		aec->ae[i].handle = dlopen (aec->ae[i].sopath, RTLD_NOW);
		if (!aec->ae[i].handle)
		{
			pthread_mutex_unlock(&(aec->rackmutex));
			printf("%s\n", dlerror());
			return -1;
		}

		aec->ae[i].aef_init = dlsym(aec->ae[i].handle, "aef_init");
		if ((error = dlerror()) != NULL)
		{
			pthread_mutex_unlock(&(aec->rackmutex));
			printf("aef_init, %s\n", error);
			return -2;
		}

		aec->ae[i].aef_setparameter = dlsym(aec->ae[i].handle, "aef_setparameter");
		if ((error = dlerror()) != NULL)
		{
			pthread_mutex_unlock(&(aec->rackmutex));
			printf("aef_setparameter, %s\n", error);
			return -2;
		}

		aec->ae[i].aef_getparameter = dlsym(aec->ae[i].handle, "aef_getparameter");
		if ((error = dlerror()) != NULL)
		{
			pthread_mutex_unlock(&(aec->rackmutex));
			printf("aef_getparameter, %s\n", error);
			return -2;
		}

		aec->ae[i].aef_process = dlsym(aec->ae[i].handle, "aef_process");
		if ((error = dlerror()) != NULL)
		{
			pthread_mutex_unlock(&(aec->rackmutex));
			printf("aef_process, %s\n", error);
			return -2;
		}

		aec->ae[i].aef_reinit = dlsym(aec->ae[i].handle, "aef_reinit");
		if ((error = dlerror()) != NULL)
		{
			pthread_mutex_unlock(&(aec->rackmutex));
			printf("aef_reinit, %s\n", error);
			return -2;
		}

		aec->ae[i].aef_close = dlsym(aec->ae[i].handle, "aef_close");
		if ((error = dlerror()) != NULL)
		{
			pthread_mutex_unlock(&(aec->rackmutex));
			printf("aef_close, %s\n", error);
			return -2;
		}
	}
	else
	{
		pthread_mutex_unlock(&(aec->rackmutex));
		printf("rack full\n");
		return -3;
	}

	audioeffect_init(&(aec->ae[i]), id);

	pthread_mutex_unlock(&(aec->rackmutex));
	return i;
}

/*
void audioeffectchain_process(audioeffectchain *aec, char *inbuffer, int inbuffersize)
{
	int effect;
	audioeffect *ae;

	pthread_mutex_lock(&(aec->rackmutex));
	if (aec->id)
	{
		for(effect=0;effect<aec->effects;effect++)
		{
			ae = &(aec->ae[effect]); 
			audioeffect_process(ae, (uint8_t*)inbuffer, inbuffersize);
		}
	}
	pthread_mutex_unlock(&(aec->rackmutex));
}
*/
void audioeffectchain_process(audioeffectchain *aec, char *inbuffer, int inbuffersize)
{
	int effect, i;
	audioeffect *ae;

	pthread_mutex_lock(&(aec->rackmutex));
	if (aec->id)
	{
		for(i=0;i<aec->effects;i++)
		{
			effect = aec->aeorder[i];
			if (effect==-1)
				break;
			ae = &(aec->ae[effect]); 
			audioeffect_process(ae, (uint8_t*)inbuffer, inbuffersize);
		}
	}
	pthread_mutex_unlock(&(aec->rackmutex));
}

void audioeffectchain_unloadeffect(audioeffectchain *aec, int effect)
{
	pthread_mutex_lock(&(aec->rackmutex));
	if (aec->ae[effect].handle)
	{
		audioeffect_close(&(aec->ae[effect]));
		dlclose(aec->ae[effect].handle);
		aec->ae[effect].handle = NULL;
		audioeffectchain_unorder(aec, effect);
	}
	pthread_mutex_unlock(&(aec->rackmutex));
}

void audioeffectchain_close(audioeffectchain *aec)
{
	if (aec->id)
	{
		if (get_devicetype(aec->tp.device)==hardwaredevice)
			audioeffectchain_terminate_thread(aec);
		else
			audioeffectchain_terminate_thread_ffmpeg(aec);

		pthread_mutex_lock(&(aec->rackmutex));
		aec->id = 0;
		pthread_mutex_unlock(&(aec->rackmutex));
	}
}
