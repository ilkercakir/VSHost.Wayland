/*
 * VideoPlayerWidgets.c
 * 
 * Copyright 2017  <pi@raspberrypi>
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

#include "VSTMediaPlayer.h"

char* strreplace(char *src, char *search, char *replace)
{
	char *p;
	char *front;
	char *dest;

	dest = (char*)malloc(1);
	dest[0] = '\0';
	for(front = p = src; (p = strstr(p, search)); front = p)
	{
		p[0] = '\0';
		//printf("%s\n", front);
		dest = (char*)realloc(dest, strlen(dest)+strlen(front)+1);
		strcat(dest, front);

		dest = (char*)realloc(dest, strlen(dest)+strlen(replace)+1);
		strcat(dest, replace);

		p += strlen(search);
	}
	//printf("%s\n", front);
	dest = (char*)realloc(dest, strlen(dest)+strlen(front)+1);
	strcat(dest, front);
	
	//printf("\n%s\n", dest);
	return dest;
}

char* strlastpart(char *src, char *search, int lowerupper)
{
	char *p;
	char *q;
	int i;

	q = &src[strlen(src)];
	for(p = src; (p = strstr(p, search)); p += strlen(search))
	{
		q = p;
	}
	switch (lowerupper)
	{
		case 0:
			break;
		case 1:
			for(i=0;q[i];i++) q[i] = tolower(q[i]);
			break;
		case 2:
			for(i=0;q[i];i++) q[i] = toupper(q[i]);
			break;
	}
	
	return q;
}

static int nomediafile(char *filepath)
{
	return
	(
		strcmp(strlastpart(filepath, ".", 1), ".mp3") &&
		strcmp(strlastpart(filepath, ".", 1), ".m4a") &&
		strcmp(strlastpart(filepath, ".", 1), ".wav") &&
		strcmp(strlastpart(filepath, ".", 1), ".flac") &&

		strcmp(strlastpart(filepath, ".", 1), ".avi") &&
		strcmp(strlastpart(filepath, ".", 1), ".mp4") &&
		strcmp(strlastpart(filepath, ".", 1), ".mov") &&
		strcmp(strlastpart(filepath, ".", 1), ".mkv") &&
		strcmp(strlastpart(filepath, ".", 1), ".mpg") &&
		strcmp(strlastpart(filepath, ".", 1), ".vob") &&
		strcmp(strlastpart(filepath, ".", 1), ".wmv") &&
		strcmp(strlastpart(filepath, ".", 1), ".webm")
	);
}

void init_playlistparams(playlistparams *plparams, vpwidgets *vpw, int vqMaxLength, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, unsigned int cqbufferframes, int thread_count)
{
	plparams->vpw = vpw;
	plparams->vqMaxLength = vqMaxLength;
	plparams->format = format;
	plparams->rate = rate;
	plparams->channels = channels;
	plparams->frames = frames;
	plparams->cqbufferframes = cqbufferframes;
	plparams->thread_count = thread_count;

	audioCQ_init(&(plparams->vpw->ap), plparams->format, plparams->rate, plparams->channels, plparams->frames, plparams->cqbufferframes);
}

void close_playlistparams(playlistparams *plparams)
{
	audioCQ_close(&(plparams->vpw->ap));
}

gboolean focus_iter_idle(gpointer data)
{
	GtkTreeModel *model;
	GtkTreePath *tp;
	vpwidgets *vpw = (vpwidgets *)data;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	tp = gtk_tree_model_get_path(model, &(vpw->iter));
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(vpw->listview), tp, NULL, FALSE);
	gtk_tree_path_free(tp);

	return(FALSE);
}

gboolean play_next(vpwidgets *vpw)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(vpw->listview));

	if (gtk_tree_selection_get_selected(selection, &model, &(vpw->iter)))
	{
		if (gtk_tree_model_iter_next(model, &(vpw->iter)))
		{
			gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
		}
		else
		{
			if (gtk_tree_model_iter_nth_child(model, &(vpw->iter), NULL, 0))
			{
				gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
			}
			else
			{
				printf("no entries\n");
				return FALSE;
			}
		}
	}
	if (vpw->vp.now_playing)
	{
		g_free(vpw->vp.now_playing);
		vpw->vp.now_playing = NULL;
	}
	gtk_tree_model_get(model, &(vpw->iter), COL_FILEPATH, &(vpw->vp.now_playing), -1);
//g_print("Next %s\n", now_playing);

	return TRUE;
}

gboolean play_prev(vpwidgets *vpw)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(vpw->listview));

	gtk_tree_selection_get_selected(selection, &model, &(vpw->iter));
	if (gtk_tree_model_iter_previous(model, &(vpw->iter)))
	{
		//gtk_tree_selection_select_iter(selection, &(vpw->iter));
		gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
	}
	else
	{
		gint nodecount = gtk_tree_model_iter_n_children(model, NULL);
		if (nodecount)
		{
			if (gtk_tree_model_iter_nth_child(model, &(vpw->iter), NULL, nodecount-1))
			{
				//gtk_tree_selection_select_iter(selection, &(vpw->iter));
				gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
			}
			else
			{
				printf("no entries\n");
				return FALSE;
			}
		}
	}

	if (vpw->vp.now_playing)
	{
		g_free(vpw->vp.now_playing);
		vpw->vp.now_playing = NULL;
	}
	gtk_tree_model_get(model, &(vpw->iter), COL_FILEPATH, &(vpw->vp.now_playing), -1);
	//g_print("Next %s\n", now_playing);

	return TRUE;
}

gboolean set_upper(gpointer data)
{
	vpwidgets *vpw = (vpwidgets *)data;
	videoplayer *v = &(vpw->vp);

	gtk_adjustment_set_value(vpw->hadjustment, 0);
	if (v->videoduration)
		gtk_adjustment_set_upper(vpw->hadjustment, v->videoduration);
	else
		gtk_adjustment_set_upper(vpw->hadjustment, v->audioduration);
//printf("%f, %f, %f\n", gtk_adjustment_get_value(vpw->hadjustment), gtk_adjustment_get_lower(vpw->hadjustment), gtk_adjustment_get_upper(vpw->hadjustment));
	return FALSE;
}

gboolean statusbar_msg(gpointer data)
{
	vpwidgets *vpw = (vpwidgets *)data;
	videoplayer *v = &(vpw->vp);

	char msgtext[256];
	char *txt;
	int pos;
	gtk_statusbar_pop(GTK_STATUSBAR(vpw->statusbar), vpw->context_id);
	if (v->videoduration)
	{
		txt = strlastpart(v->now_playing, "/", 0);
		pos = strlen(txt);
		strcpy(msgtext, txt);
		if (pos>70)
		{
			msgtext[30] = '\0';
			strcat(msgtext, "...");
			pos -= 30;
			strcat(msgtext, txt+pos);
		}
		sprintf(vpw->msg, "%2.2f fps, %d*%d, %s (%02i:%02i)", v->frame_rate, v->pCodecCtx->width, v->pCodecCtx->height, msgtext, ((int)(v->videoduration/v->frame_rate))/60, ((int)(v->videoduration/v->frame_rate)%60));
	}
	else
	{
		txt = strlastpart(v->now_playing, "/", 0);
		pos = strlen(txt);
		strcpy(msgtext, txt);
		if (pos>100)
		{
			msgtext[45] = '\0';
			strcat(msgtext, "...");
			pos -= 45;
			strcat(msgtext, txt+pos);
		}
		//sprintf(vpw->msg, "%s (%02i:%02i)", msgtext, ((int)(v->audioduration/v->sample_rate))/60, ((int)(v->audioduration/v->sample_rate)%60));
		sprintf(vpw->msg, "%s (%02i:%02i)", msgtext, ((int)(v->audioduration/v->spk_samplingrate))/60, ((int)(v->audioduration/v->spk_samplingrate)%60));
	}
//printf("%s\n", msg);
	gchar *buff = g_strdup_printf("%s", vpw->msg);
	gtk_statusbar_push(GTK_STATUSBAR(vpw->statusbar), vpw->context_id, buff);
	g_free(buff);

	return FALSE;
}

int create_thread0_videoplayer(vpwidgets *vpw, int vqMaxLength, int thread_count)
{
	videoplayer *v = &(vpw->vp);

	int err;

	init_videoplayer(v, vpw->playerWidth, vpw->playerHeight, vqMaxLength, &(vpw->ap), thread_count);

	gint pageno;
	g_object_get((gpointer)vpw->notebook, "page", &pageno, NULL);
//printf("Selected id %d\n", pageno);
	v->decodevideo = !pageno;

	if ((err=open_now_playing(v))<0)
	{
		printf("open_now_playing() error %d\n", err);
		return(err);
	}

	err = pthread_create(&(v->tid[0]), NULL, &thread0_videoplayer, (void *)v);
	if (err)
	{}
/*
//printf("thread0_videoplayer->%d\n", 0);
	if ((err=pthread_setaffinity_np(v->tid[0], sizeof(cpu_set_t), &(v->cpu[0]))))
		printf("pthread_setaffinity_np error %d\n", err);
*/

	gdk_threads_add_idle(statusbar_msg, (void*)vpw);

	int i;
	if ((i=pthread_join(v->tid[0], NULL)))
		printf("pthread_join error, v->tid[0], %d\n", i);

	return(v->stoprequested);
}

static gpointer playlist_thread(gpointer args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	playlistparams *plp = (playlistparams*)args;

	while(1)
	{
		if (create_thread0_videoplayer(plp->vpw, plp->vqMaxLength, plp->thread_count))
			break;
		if (!play_next(plp->vpw))
			break;
	}

//printf("exiting playlist_thread\n");
	plp->vpw->retval0 = 0;
	pthread_exit(&(plp->vpw->retval0));
}

int create_playlist_thread(playlistparams *plp)
{
	int err;

	err = pthread_create(&(plp->vpw->tid), NULL, &playlist_thread, (void *)plp);
	if (err)
	{}
/*
//printf("playlist_thread->%d\n", 0);
	if ((err=pthread_setaffinity_np(plp->vpw->tid, sizeof(cpu_set_t), &(plp->vpw->cpu[0]))))
		printf("pthread_setaffinity_np error %d\n", err);
*/
	return(0);
}

int select_callback(void *data, int argc, char **argv, char **azColName) 
{
	vpwidgets *vpw = (vpwidgets*)data;
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

	gtk_list_store_append(vpw->store, &(vpw->iter));
	gtk_list_store_set(vpw->store, &(vpw->iter), COL_ID, atoi(argv[0]), COL_FILEPATH, argv[1], -1);

//g_print("%s, %s\n", argv[0], argv[1]);
	return 0;
}

static GtkTreeModel* create_and_fill_model(vpwidgets *vpw, int mode)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char *sql = NULL;
	int rc;

	vpw->store = gtk_list_store_new(NUM_COLS, G_TYPE_UINT, G_TYPE_STRING);

	switch(mode)
	{
		case 0:
			break;
		case 1:
			if((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
			{
				printf("Can't open database: %s\n", sqlite3_errmsg(db));
			}
			else
			{
//printf("Opened database successfully\n");
				sql = "SELECT * FROM mediafiles order by id;";
				if((rc = sqlite3_exec(db, sql, select_callback, (void*)vpw, &err_msg)) != SQLITE_OK)
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
		default:
			break;
	}

	return GTK_TREE_MODEL(vpw->store);
}

gboolean search_equal_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter, gpointer search_data)
{
	gboolean valid;
	gchararray gca;
	vpwidgets *vpw = (vpwidgets *)search_data;

	//printf("column %d, key %s\n", column, key);
    for(valid=gtk_tree_model_get_iter_first(model, &(vpw->iter));valid;valid=gtk_tree_model_iter_next(model, &(vpw->iter)))
	{
		gtk_tree_model_get(model, &(vpw->iter), COL_FILEPATH, &gca, -1);
		if (strstr((char*)gca, (char*)key))
		{
//printf("%s, %s\n", gca, key);
			gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
			break;
		}
		g_free(gca);
    }

	return(FALSE); // found
}

void search_destroy(gpointer data)
{
	//g_free(data);
}

static GtkWidget* create_view_and_model(vpwidgets *vpw)
{
	GtkCellRenderer *renderer;
	GtkTreeModel *model;
	GtkWidget *view;

	view = gtk_tree_view_new();

	// Column 1
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, "ID", renderer, "text", COL_ID, NULL);

	// Column 2
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(view), -1, "File Path", renderer, "text", COL_FILEPATH, NULL);

	model = create_and_fill_model(vpw, 0);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(view), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(view), COL_FILEPATH);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(view), search_equal_func, (void*)vpw, search_destroy);

/* The tree view has acquired its own reference to the model, so we can drop ours. That way the model will
   be freed automatically when the tree view is destroyed */
//g_object_unref(model);

	return view;
}

static void button1_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vpp = &(vpw->vp);
	int ret;

//g_print("Button 1 clicked\n");
	if (!(vpp->now_playing))
		return;

	if((ret=create_playlist_thread(plp))<0)
	{
		printf("create_playlist_thread error %d\n", ret);
		return;
	}

	gtk_widget_set_sensitive(vpw->button1, FALSE);
	gtk_widget_set_sensitive(vpw->button2, TRUE);
	gtk_widget_set_sensitive(vpw->button8, TRUE);
	gtk_widget_set_sensitive(vpw->button9, TRUE);
	gtk_widget_set_sensitive(vpw->button10, TRUE);

}

static void button2_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vp = &(vpw->vp);

//g_print("Button 2 clicked\n");
	request_stop_frame_reader(vp);

	int i;
	if ((i=pthread_join(vpw->tid, NULL)))
		printf("pthread_join error, vpw->tid, %d\n", i);

	gtk_widget_set_sensitive(vpw->button2, FALSE);
	gtk_widget_set_sensitive(vpw->button8, FALSE);
	gtk_widget_set_sensitive(vpw->button9, FALSE);
	gtk_widget_set_sensitive(vpw->button10, FALSE);
	gtk_widget_set_sensitive(vpw->button1, TRUE);
}

/*
static void buttonParameters_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	if (gtk_widget_get_visible(vpw->windowparm))
		gtk_widget_hide(vpw->windowparm);
	else
		gtk_widget_show_all(vpw->windowparm);
}
*/

static void button3_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	GtkTreeModel *model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	gtk_list_store_clear((GtkListStore*)model);

	gtk_tree_view_set_model(GTK_TREE_VIEW(vpw->listview), NULL); /* Detach model from view */
	g_object_unref(model);
	model = create_and_fill_model(vpw, 0); // do not insert rows
	gtk_tree_view_set_model(GTK_TREE_VIEW(vpw->listview), model); /* Re-attach model to view */

	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(vpw->listview), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(vpw->listview), COL_FILEPATH);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(vpw->listview), search_equal_func, (void*)vpw, search_destroy);
}

static void button4_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	GtkTreeModel *model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(vpw->listview));
	gtk_list_store_clear((GtkListStore*)model);

	gtk_tree_view_set_model(GTK_TREE_VIEW(vpw->listview), NULL); /* Detach model from view */
	g_object_unref(model);
	model = create_and_fill_model(vpw, 1); // insert rows from db
	gtk_tree_view_set_model(GTK_TREE_VIEW(vpw->listview), model); /* Re-attach model to view */

	gtk_tree_view_set_enable_search (GTK_TREE_VIEW(vpw->listview), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(vpw->listview), COL_FILEPATH);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(vpw->listview), search_equal_func, (void*)vpw, search_destroy);
}

void listdir(const char *name, sqlite3 *db, int *id)
{
	char *err_msg = NULL;
	char sql[1024];
	int rc;
	char sid[10];
	char path[1024];
	char *strname;
	char *strdest;

	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(name)))
		return;
	if (!(entry = readdir(dir)))
		return;

	do
	{
		if (entry->d_type == DT_DIR)
		{
			int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
			path[len] = 0;
			if ((!strcmp(entry->d_name, ".")) || (!strcmp(entry->d_name, "..")))
				continue;
//printf("%*s[%s]\n", level*2, "", entry->d_name);
			listdir(path, db, id);
		}
		else
		{
//printf("%*s- %s/%s\n", level*2, "", name, entry->d_name);
			if (nomediafile(entry->d_name))
				continue;

			(*id)++;
			sprintf(sid, "%d", *id);
			sql[0] = '\0';
			strcat(sql, "INSERT INTO mediafiles VALUES(");
			strcat(sql, sid);
			strcat(sql, ", '");
			strcat(sql, name);
			strcat(sql, "/");
				//strcat(sql, entry->d_name);
				strname=(char*)malloc(sizeof(entry->d_name)+1);
				strcpy(strname, entry->d_name);
				strdest = strreplace(strname, "'", "''");
				free(strname);
				strcat(sql, strdest);
				free(strdest);
			strcat(sql, "');");
//printf("%s\n", sql);
			if((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
			{
				printf("Failed to insert data, %s\n", err_msg);
				sqlite3_free(err_msg);
			}
			else
			{
// success
			}
		}
	}
	while((entry = readdir(dir)));
	closedir(dir);
}

static void button6_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vpp = &(vpw->vp);

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;

	char *err_msg = NULL;
	sqlite3 *db;
	char *sql;
	int rc;
	int id;

	if (!vpp->catalog_folder)
	{
		g_free(vpp->catalog_folder);
		vpp->catalog_folder = NULL;
	}

	dialog = gtk_file_chooser_dialog_new("Open Folder for Catalog", GTK_WINDOW(vpw->vpwindow), action, "Cancel", GTK_RESPONSE_CANCEL, "Select Folder", GTK_RESPONSE_ACCEPT, NULL);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		vpp->catalog_folder = gtk_file_chooser_get_filename(chooser);

		if ((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
		{
			printf("Can't open database: %s\n", sqlite3_errmsg(db));
		}
		else
		{
//printf("Opened database successfully\n");
			sql = "DELETE FROM mediafiles;";
			if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK)
			{
				printf("Failed to delete data, %s\n", err_msg);
				sqlite3_free(err_msg);
			}
			else
			{
// success
			}
			id = 0;
			listdir(vpp->catalog_folder, db, &id);
		}
		sqlite3_close(db);

	}
	gtk_widget_destroy (dialog);
}

int select_add_callback(void *data, int argc, char **argv, char **azColName) 
{
	vpwidgets *vpw = (vpwidgets*)data;

	vpw->last_id = atoi(argv[0]);
	return 0;
}

int select_add_lastid(vpwidgets *vpw)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char *sql = NULL;
	int rc;

	vpw->last_id = 0;
	if((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sql = "SELECT max(id) as id FROM mediafiles;";
		if((rc = sqlite3_exec(db, sql, select_add_callback, (void*)vpw, &err_msg)) != SQLITE_OK)
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

	return(vpw->last_id);
}

static void button7_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	char *err_msg = NULL;
	sqlite3 *db;
	int rc;
	int id;
	char sql[1024];
	char sid[20];
	char *strname;
	char *strdest;

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GSList *chosenfile;

	dialog = gtk_file_chooser_dialog_new("Add File", GTK_WINDOW(vpw->vpwindow), action, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_select_multiple(chooser, TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		if ((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
		{
			printf("Can't open database: %s\n", sqlite3_errmsg(db));
		}
		else
		{
			id = select_add_lastid(vpw);
			GSList *filelist = gtk_file_chooser_get_filenames(chooser);
			for(chosenfile=filelist;chosenfile;chosenfile=chosenfile->next)
			{
//printf("%s\n", (char*)chosenfile->data);
				if (nomediafile((char*)chosenfile->data))
					continue;

				id++;
				sprintf(sid, "%d", id);
				sql[0] = '\0';
				strcat(sql, "INSERT INTO mediafiles VALUES(");
				strcat(sql, sid);
				strcat(sql, ", '");
					//strcat(sql, (char*)chosenfile->data));
					strname=(char*)malloc(1024);
					strcpy(strname, (char*)chosenfile->data);
					strdest = strreplace(strname, "'", "''");
					free(strname);
					strcat(sql, strdest);
					free(strdest);
				strcat(sql, "');");
//printf("%s\n", sql);
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
			g_slist_free(filelist);
			button3_clicked(vpw->button3, plp);
			button4_clicked(vpw->button4, plp);
		}
		sqlite3_close(db);
	}
	gtk_widget_destroy(dialog);
}

static void button8_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
//g_print("Button 8 clicked\n");
	button2_clicked(vpw->button2, data);
	if (play_next(vpw))
	{
		button1_clicked(vpw->button1, data);
		gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
	}
}

static void button9_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
//g_print("Button 9 clicked\n");
	button2_clicked(vpw->button2, data);
	if (play_prev(vpw))
	{
		button1_clicked(vpw->button1, data);
		gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
	}
}

static void button10_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vpp = &(vpw->vp);
//g_print("Button 10 clicked\n");
	if (vpp->vpq.playerstatus==PLAYING)
	{
		pthread_mutex_lock(&(vpp->seekmutex));
		vpp->vpq.playerstatus = PAUSED;
		pthread_mutex_unlock(&(vpp->seekmutex));

		gtk_button_set_label(GTK_BUTTON(vpw->button10), "Resume");
		gtk_widget_set_sensitive(vpw->button2, FALSE);
		gtk_widget_set_sensitive(vpw->button8, FALSE);
		gtk_widget_set_sensitive(vpw->button9, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(vpw->button2, TRUE);
		gtk_widget_set_sensitive(vpw->button8, TRUE);
		gtk_widget_set_sensitive(vpw->button9, TRUE);
		gtk_button_set_label(GTK_BUTTON(vpw->button10), "Pause");

		pthread_mutex_lock(&(vpp->seekmutex));
		vpp->vpq.playerstatus = PLAYING;
		pthread_cond_signal(&(vpp->pausecond)); // Should wake up *one* thread
		pthread_mutex_unlock(&(vpp->seekmutex));
	}
}

static void button11_clicked(GtkWidget *button, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	GSList *chosenfile;

	int i = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(vpw->store), NULL); // rows

	dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(vpw->vpwindow), action, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_select_multiple(chooser, TRUE);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		//button3_clicked(vpw->button3, plp); // Clear list
		GSList *filelist = gtk_file_chooser_get_filenames(chooser);
		for(chosenfile=filelist;chosenfile;chosenfile=chosenfile->next)
		{
			if (nomediafile((char*)chosenfile->data))
				continue;
//printf("%s\n", (char*)chosenfile->data);
			gtk_list_store_append(vpw->store, &(vpw->iter));
			gtk_list_store_set(vpw->store, &(vpw->iter), COL_ID, i++, COL_FILEPATH, chosenfile->data, -1);
		}
		g_slist_free(filelist);
	}
	gtk_widget_destroy(dialog);
}

gboolean scale_pressed(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	vpwidgets *vpw = (vpwidgets*)user_data;
	videoplayer *vpp = &(vpw->vp);

	if (vpp->vpq.playerstatus==PLAYING)
	{
		pthread_mutex_lock(&(vpp->seekmutex));
		vpp->vpq.playerstatus = PAUSED;
		pthread_mutex_unlock(&(vpp->seekmutex));

		vpw->sliding = 1;
		vpw->hscaleupd = FALSE;
	}
	return FALSE;
}

gboolean scale_released(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	vpwidgets *vpw = (vpwidgets*)user_data;
	videoplayer *vpp = &(vpw->vp);
	int64_t seek_ts;

	double value = gtk_range_get_value(GTK_RANGE(widget));
//printf("Released value: %f\n", value);

	if (vpw->sliding)
	{
		if (vpp->videoduration)
		{
			AVStream *st = vpp->pFormatCtx->streams[vpp->videoStream];
			double seektime = value / vpp->frame_rate; // seconds
//printf("Seek time %5.2f\n", seektime);
			int flgs = AVSEEK_FLAG_ANY;
			seek_ts = (seektime * (st->time_base.den)) / (st->time_base.num);
			if (av_seek_frame(vpp->pFormatCtx, vpp->videoStream, seek_ts, flgs) < 0)
			{
				printf("Failed to seek Video\n");
			}
			else
			{
				avcodec_flush_buffers(vpp->pCodecCtx);
				if (vpp->audioStream!=-1)
					avcodec_flush_buffers(vpp->pCodecCtxA);

				//vq_drain(&vq);
				//aq_drain(&aq);
			}

			pthread_mutex_lock(&(vpp->framemutex));
			vpp->now_decoding_frame = (int64_t)value;
			pthread_mutex_unlock(&(vpp->framemutex));
		}
		else
		{
			AVStream *st = vpp->pFormatCtx->streams[vpp->audioStream];
			double seektime = value / vpp->sample_rate; // seconds
//printf("Seek time %5.2f\n", seektime);
			int flgs = AVSEEK_FLAG_ANY;
			seek_ts = (seektime * (st->time_base.den)) / (st->time_base.num);
//printf("seek_ts %lld\n", seek_ts);
			if (av_seek_frame(vpp->pFormatCtx, vpp->audioStream, seek_ts, flgs) < 0)
			{
				printf("Failed to seek Audio\n");
			}
			else
			{
				avcodec_flush_buffers(vpp->pCodecCtxA);
			}
			pthread_mutex_lock(&(vpp->framemutex));
			vpp->now_playing_frame = (int64_t)value;
			pthread_mutex_unlock(&(vpp->framemutex));
		}

		vpw->sliding = 0;
		vpw->hscaleupd = TRUE;

		pthread_mutex_lock(&(vpp->seekmutex));
		vpp->vpq.playerstatus = PLAYING;
		pthread_cond_signal(&(vpp->pausecond)); // Should wake up *one* thread
		pthread_mutex_unlock(&(vpp->seekmutex));
	}
	return FALSE;
}

void listview_onRowActivated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
	playlistparams *plp = (playlistparams*)userdata;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vpp = &(vpw->vp);
	GtkTreeModel *model;
	GtkTreeIter iter;
	//gchar *s;

	press_vp_resume_button(plp); // Press resume if paused 
	press_vp_stop_button(plp); // Press stop if playing

//g_print("double-clicked\n");
	model = gtk_tree_view_get_model(treeview);
	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		//s=gtk_tree_path_to_string(path);
		//printf("%s\n",s);
		//g_free(s);
		if (vpp->now_playing)
		{
			g_free(vpp->now_playing);
			vpp->now_playing = NULL;
		}
		gtk_tree_model_get(model, &iter, COL_FILEPATH, &(vpp->now_playing), -1);
//g_print ("Double-clicked path %s\n", vpp->now_playing);

		button1_clicked(vpw->button1, userdata);
	}
}

/* Called when the windows are realized */
static void vp_realize_cb(GtkWidget *widget, gpointer data)
{
//	vpwidgets *vpw = (vpwidgets*)data;
}

static gboolean vp_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    /* If you return FALSE in the "delete-event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */
//g_print ("delete event occurred\n");
    return TRUE;
}

static void vp_destroy(GtkWidget *widget, gpointer data)
{
//printf("vp_destroy\n");
    //gtk_main_quit();
}

void page_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data)
{
	vpwidgets *vpw = (vpwidgets *)user_data;

//printf("switched to page %d\n", page_num);
	vpw->vp.decodevideo = !page_num;
}

gboolean update_hscale(gpointer data)
{
	vpwidgets *vpw = (vpwidgets*)data;
	videoplayer *vp = &(vpw->vp);

	if (vp->hscaleupd)
		gtk_adjustment_set_value(vpw->hadjustment, vp->now_playing_frame);
	return FALSE;
}

void drag_data_received_da_event(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint time, gpointer data)
{
	playlistparams *plp = (playlistparams*)data;
	vpwidgets *vpw = plp->vpw;
	videoplayer *vp = &(vpw->vp);
	videoplayerqueue *vpq = &(vp->vpq);
	int itemcount, currentpage;

//printf("drag_data_received\n");		
	if (!(currentpage = gtk_notebook_get_current_page(GTK_NOTEBOOK(vpw->notebook))))
		button3_clicked(vpw->button3, plp); // Clear list
	itemcount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(vpw->store), NULL); // rows

	gchar **uris = gtk_selection_data_get_uris(selection_data);
	for (gchar **uris_iter = uris; uris_iter && *uris_iter; ++uris_iter)
	{
		gchar *path = g_filename_from_uri(*uris_iter, NULL, NULL);
//printf("%s\n", path);
		if (nomediafile(path))
		{}
		else
		{
			gtk_list_store_append(vpw->store, &(vpw->iter));
			gtk_list_store_set(vpw->store, &(vpw->iter), COL_ID, itemcount++, COL_FILEPATH, path, -1);
		}
		g_free(path);
	}
	g_strfreev(uris);

//	if (itemcount)
//	{
		if (vpq->playerstatus==PLAYING)
			button2_clicked(vpw->button2, (void*)plp);
		if (vpq->playerstatus==IDLE)
		{
			if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(vpw->store), &(vpw->iter), NULL, 0))
			{
				if (vp->now_playing)
				{
					g_free(vp->now_playing);
					vp->now_playing = NULL;
				}
				gtk_tree_model_get(GTK_TREE_MODEL(vpw->store), &(vpw->iter), COL_FILEPATH, &(vp->now_playing), -1);
				gdk_threads_add_idle(focus_iter_idle, (void*)vpw);
				gtk_notebook_set_current_page(GTK_NOTEBOOK(vpw->notebook), 0);
				button1_clicked(vpw->button1, (void*)plp);
			}
		}
//	}
}

gboolean drag_drop_da_event(GtkWidget *widget, GdkDragContext*context, gint x, gint y, guint time, gpointer data)
{
//printf("drag_drop\n");
	return FALSE;
}

void drag_motion_da_event(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer data)
{
//printf("drag_motion\n");
}

void drag_leave_da_event(GtkWidget *widget, GdkDragContext *context, guint time, gpointer data)
{
//printf("drag_leave\n");
}

void new_drawingarea(playlistparams *plp)
{
	vpwidgets *vpw = plp->vpw;
	oglidle *oi = (oglidle*)&(vpw->vp.oi);
	oglstate *ogl = &(oi->ogl);

	vpw->drawingarea = gtk_drawing_area_new();
	vpw->vp.playerWidth = vpw->playerWidth;
	vpw->vp.playerHeight = vpw->playerHeight;
	vpw->vp.drawingarea = vpw->drawingarea;

	ogl->fmt = RGBA;
	ogl->width = vpw->playerWidth;
	ogl->height = vpw->playerHeight;
	ogl->linewidth = vpw->playerWidth;
	ogl->codecheight = vpw->playerHeight;

	oi->widget = vpw->drawingarea;

	//gtk_widget_set_double_buffered(vpw->drawingarea, FALSE);
	gtk_widget_set_size_request(vpw->drawingarea, vpw->playerWidth, vpw->playerHeight);
	g_signal_connect(vpw->drawingarea, "realize", G_CALLBACK(realize_da_event), (void*)oi);
	g_signal_connect(vpw->drawingarea, "draw", G_CALLBACK(draw_da_event), (void*)oi);
	//g_signal_connect(vpw->drawingarea, "size-allocate", G_CALLBACK(size_allocate_da_event), (void*)oi);
	g_signal_connect(vpw->drawingarea, "destroy", G_CALLBACK(destroy_da_event), (void*)oi);

	gtk_drag_dest_set(vpw->drawingarea, GTK_DEST_DEFAULT_ALL, vpw->target_entries, G_N_ELEMENTS(vpw->target_entries), GDK_ACTION_COPY | GDK_ACTION_MOVE );
	g_signal_connect(vpw->drawingarea, "drag_data_received", G_CALLBACK(drag_data_received_da_event), (void*)plp);
	g_signal_connect(vpw->drawingarea, "drag_drop", G_CALLBACK(drag_drop_da_event), NULL);
	g_signal_connect(vpw->drawingarea, "drag_motion", G_CALLBACK(drag_motion_da_event), NULL);
	g_signal_connect(vpw->drawingarea, "drag_leave", G_CALLBACK(drag_leave_da_event), NULL);
}

void init_target_list(vpwidgets *vpw)
{
	int i;

	GtkTargetEntry target_entries[3] = 
	{
		{ "text/html", GTK_TARGET_OTHER_APP, text_html },
		{ "STRING", GTK_TARGET_OTHER_APP, string },
		{ "text/uri-list", GTK_TARGET_OTHER_APP, string }
	};

	for(i=0;i<G_N_ELEMENTS(target_entries);i++)
		vpw->target_entries[i] = target_entries[i];
}

void init_videoplayerwidgets(playlistparams *plp, int playWidth, int playHeight)
{
	vpwidgets *vpw = plp->vpw;

	init_target_list(vpw);
	
	vpw->vp.now_playing = NULL; // Video Player
	vpw->vp.vpq.playerstatus = IDLE;
	vpw->vp.vpwp = (void*)vpw;
	vpw->vp.thread_count = plp->thread_count;

	vpw->playerWidth = playWidth;
	vpw->playerHeight = playHeight;
	vpw->sliding = 0;

	int i;
	for(i=0;i<4;i++)
	{
		CPU_ZERO(&(vpw->cpu[i]));
		CPU_SET(i, &(vpw->cpu[i]));
	}

    /* create a new window */
	vpw->vpwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(vpw->vpwindow), GTK_WIN_POS_CENTER);
	/* Sets the border width of the window. */
	gtk_container_set_border_width(GTK_CONTAINER(vpw->vpwindow), 2);
	//gtk_widget_set_size_request(vpw->vpwindow, 100, 100);
	gtk_window_set_title(GTK_WINDOW(vpw->vpwindow), "Video Player");
	//gtk_window_set_resizable(GTK_WINDOW(vpw->vpwindow), FALSE);

	/* When the window is given the "delete-event" signal (this is given
     * by the window manager, usually by the "close" option, or on the
     * titlebar), we ask it to call the delete_event () function
     * as defined above. The data passed to the callback
     * function is NULL and is ignored in the callback function. */
	g_signal_connect(vpw->vpwindow, "delete-event", G_CALLBACK(vp_delete_event), (void*)vpw);
    
	/* Here we connect the "destroy" event to a signal handler.  
     * This event occurs when we call gtk_widget_destroy() on the window,
     * or if we return FALSE in the "delete-event" callback. */
	g_signal_connect(vpw->vpwindow, "destroy", G_CALLBACK(vp_destroy), (void*)vpw);

	g_signal_connect(vpw->vpwindow, "realize", G_CALLBACK(vp_realize_cb), (void*)vpw);

// box1 contents begin
// vertical box
	vpw->box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

// egl Wayland drawing area
	new_drawingarea(plp);
	gtk_box_pack_start(GTK_BOX(vpw->box1), vpw->drawingarea, TRUE, TRUE, 0);

// horizontal scale
	vpw->hadjustment = gtk_adjustment_new(50, 0, 100, 1, 10, 0);
	vpw->hscale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT(vpw->hadjustment));
	gtk_scale_set_digits(GTK_SCALE(vpw->hscale), 0);
	//g_signal_connect(hscale, "value-changed", G_CALLBACK(hscale_adjustment), NULL);
	g_signal_connect(vpw->hscale, "button-press-event", G_CALLBACK(scale_pressed), (void*)vpw);
	g_signal_connect(vpw->hscale, "button-release-event", G_CALLBACK(scale_released), (void*)vpw);
	gtk_container_add(GTK_CONTAINER(vpw->box1), vpw->hscale);

// horizontal button box
    vpw->button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout((GtkButtonBox *)vpw->button_box, GTK_BUTTONBOX_START);
    gtk_container_add(GTK_CONTAINER(vpw->box1), vpw->button_box);

// button prev
	vpw->button9 = gtk_button_new_with_label("Prev");
	gtk_widget_set_sensitive (vpw->button9, FALSE);
	g_signal_connect(GTK_BUTTON(vpw->button9), "clicked", G_CALLBACK(button9_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button9);

// button play
	vpw->button1 = gtk_button_new_with_label("Play");
	g_signal_connect(GTK_BUTTON(vpw->button1), "clicked", G_CALLBACK(button1_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button1);

// button pause/resume
	vpw->button10 = gtk_button_new_with_label("Pause");
	gtk_widget_set_sensitive (vpw->button10, FALSE);
	g_signal_connect(GTK_BUTTON(vpw->button10), "clicked", G_CALLBACK(button10_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button10);

// button next
	vpw->button8 = gtk_button_new_with_label("Next");
	gtk_widget_set_sensitive (vpw->button8, FALSE);
	g_signal_connect(GTK_BUTTON(vpw->button8), "clicked", G_CALLBACK(button8_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button8);

// button stop
	vpw->button2 = gtk_button_new_with_label("Stop");
	gtk_widget_set_sensitive(vpw->button2, FALSE);
	g_signal_connect(GTK_BUTTON(vpw->button2), "clicked", G_CALLBACK(button2_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box), vpw->button2);
// box1 contents end

// box2 contents begin
	vpw->box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

	vpw->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(vpw->scrolled_window), 10);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(vpw->scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_widget_set_size_request(vpw->scrolled_window, vpw->playerWidth, vpw->playerHeight);
	//gtk_container_add(GTK_CONTAINER(vpw->box2), vpw->scrolled_window);
	gtk_box_pack_start(GTK_BOX(vpw->box2), vpw->scrolled_window, TRUE, TRUE, 0);

	vpw->listview = create_view_and_model(vpw);
	g_signal_connect(vpw->listview, "row-activated", (GCallback)listview_onRowActivated, (void*)plp);
	gtk_drag_dest_set(vpw->listview, GTK_DEST_DEFAULT_ALL, vpw->target_entries, G_N_ELEMENTS(vpw->target_entries), GDK_ACTION_COPY | GDK_ACTION_MOVE );
	g_signal_connect(vpw->listview, "drag_data_received", G_CALLBACK(drag_data_received_da_event), (void*)plp);
	g_signal_connect(vpw->listview, "drag_drop", G_CALLBACK(drag_drop_da_event), NULL);
	g_signal_connect(vpw->listview, "drag_motion", G_CALLBACK(drag_motion_da_event), NULL);
	g_signal_connect(vpw->listview, "drag_leave", G_CALLBACK(drag_leave_da_event), NULL);
	gtk_container_add(GTK_CONTAINER(vpw->scrolled_window), vpw->listview);

	vpw->button_box2 = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout((GtkButtonBox*)vpw->button_box2, GTK_BUTTONBOX_START);
	gtk_container_add(GTK_CONTAINER(vpw->box2), vpw->button_box2);

	vpw->button3 = gtk_button_new_with_label("Clear");
	g_signal_connect(GTK_BUTTON(vpw->button3), "clicked", G_CALLBACK(button3_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button3);

	vpw->button6 = gtk_button_new_with_label("Catalog");
	g_signal_connect(GTK_BUTTON(vpw->button6), "clicked", G_CALLBACK(button6_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button6);

	vpw->button7 = gtk_button_new_with_label("Add File");
	g_signal_connect(GTK_BUTTON(vpw->button7), "clicked", G_CALLBACK(button7_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button7);

	vpw->button4 = gtk_button_new_with_label("Load");
	g_signal_connect(GTK_BUTTON(vpw->button4), "clicked", G_CALLBACK(button4_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button4);

	vpw->button11 = gtk_button_new_with_label("Open Files");
	g_signal_connect(GTK_BUTTON(vpw->button11), "clicked", G_CALLBACK(button11_clicked), (void*)plp);
	gtk_container_add(GTK_CONTAINER(vpw->button_box2), vpw->button11);

// box2 contents end

// vertical box
	vpw->playerbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
	gtk_container_add(GTK_CONTAINER(vpw->vpwindow), vpw->playerbox);

	vpw->notebook = gtk_notebook_new();
	vpw->nbpage1 = gtk_label_new("Player");
	gtk_notebook_append_page(GTK_NOTEBOOK(vpw->notebook), vpw->box1, vpw->nbpage1);
	vpw->nbpage2 = gtk_label_new("Playlist");
	gtk_notebook_append_page(GTK_NOTEBOOK(vpw->notebook), vpw->box2, vpw->nbpage2);
	g_signal_connect(GTK_NOTEBOOK(vpw->notebook), "switch-page", G_CALLBACK(page_switched), (void*)vpw);
	//gtk_container_add(GTK_CONTAINER(vpw->playerbox), vpw->notebook);
	gtk_box_pack_start(GTK_BOX(vpw->playerbox), vpw->notebook, TRUE, TRUE, 0);

    vpw->statusbar = gtk_statusbar_new();
    vpw->context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(vpw->statusbar), "statusbar context");
    gtk_container_add(GTK_CONTAINER(vpw->playerbox), vpw->statusbar);
    gtk_statusbar_push(GTK_STATUSBAR(vpw->statusbar), vpw->context_id, "");

	gtk_widget_show_all(vpw->vpwindow);
}

void press_vp_stop_button(playlistparams *plp)
{
	vpwidgets *vpw = plp->vpw;
	videoplayer *vp = &(vpw->vp);

	if (vp->vpq.playerstatus!=IDLE)
	{
		button2_clicked(vpw->button2, plp);
	}
}

void press_vp_resume_button(playlistparams *plp)
{
	vpwidgets *vpw = plp->vpw;
	videoplayer *vp = &(vpw->vp);

	if (vp->vpq.playerstatus==PAUSED)
	{
		button10_clicked(vpw->button10, plp);
	}
}

void close_videoplayerwidgets(playlistparams *plp)
{
	vpwidgets *vpw = plp->vpw;

	press_vp_resume_button(plp); // Press resume if paused 
	press_vp_stop_button(plp); // Press stop if playing

	gtk_widget_destroy(vpw->vpwindow);
}

gboolean setnotebooktab1(gpointer data)
{
	vpwidgets *vpw = (vpwidgets *)data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(vpw->notebook), 1);
	return FALSE;
}
