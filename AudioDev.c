/*
 * AudioDev.c
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

#include "AudioDev.h"

void populate_card_list(GtkWidget *comboinputdev, GtkWidget *combooutputdev)
{
	int status, count = 0;
	int card = -1;  // use -1 to prime the pump of iterating through card list
	char* longname  = NULL;
	char* shortname = NULL;
	char name[32];
	char devicename[32];
	char devicenamedisplayed[128];
	//const char* subname;
	snd_ctl_t *ctl;
	snd_pcm_info_t *info;
	int device;
	int sub, foundsub, inpreset=0, outpreset=0;

	do
	{
		if ((status = snd_card_next(&card)) < 0)
		{
			printf("cannot determine card number: %s\n", snd_strerror(status));
			break;
		}
		if (card<0) break;
		//printf("Card %d:", card);
		if ((status = snd_card_get_name(card, &shortname)) < 0)
		{
			printf("cannot determine card shortname: %s\n", snd_strerror(status));
			break;
		}
		if ((status = snd_card_get_longname(card, &longname)) < 0)
		{
			printf("cannot determine card longname: %s\n", snd_strerror(status));
			break;
		}
		//printf("\tLONG NAME:  %s\n", longname);
		//printf("\tSHORT NAME: %s\n", shortname);

		sprintf(name, "hw:%d", card);
		if ((status = snd_ctl_open(&ctl, name, 0)) < 0)
		{
			printf("cannot open control for card %d: %s\n", card, snd_strerror(status));
			return;
		}

		device = -1;
		do
		{
			status = snd_ctl_pcm_next_device(ctl, &device);
			if (status < 0)
			{
				printf("cannot determine device number: %s\n", snd_strerror(status));
				break;
			}
			if (device<0) break;
			//printf("Device %s,%d\n", name, device);
			snd_pcm_info_malloc(&info);
			snd_pcm_info_set_device(info, (unsigned int)device);

			//snd_pcm_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
			snd_pcm_info_set_stream(info, SND_PCM_STREAM_CAPTURE);
			snd_ctl_pcm_info(ctl, info);
			int subs_in = snd_pcm_info_get_subdevices_count(info);
			//printf("Input subdevices : %d\n", subs_in);
			for(sub=0,foundsub=0;sub<subs_in;sub++)
			{
				snd_pcm_info_set_subdevice(info, sub);
				if ((status = snd_ctl_pcm_info(ctl, info)) < 0)
				{
					//printf("cannot get pcm information %d:%d:%d: %s\n", card, device, sub, snd_strerror(status));
					continue;
				}
				//subname = snd_pcm_info_get_subdevice_name(info);
				//printf("Subdevice %d name : %s\n", sub, subname);
				foundsub = 1;
			}
			if (foundsub)
			{
				sprintf(devicename, "hw:%d,%d", card, device);
				sprintf(devicenamedisplayed, "%s %s", shortname, devicename);
				//gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(comboinputdev), devicename, shortname);
				gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(comboinputdev), devicename, devicenamedisplayed);
				if (!inpreset)
				{
					inpreset = 1;
					g_object_set((gpointer)comboinputdev, "active-id", devicename, NULL);
				}
			}

			snd_pcm_info_set_stream(info, SND_PCM_STREAM_PLAYBACK);
			snd_ctl_pcm_info(ctl, info);
			int subs_out = snd_pcm_info_get_subdevices_count(info);
			//printf("Output subdevices : %d\n", subs_out);
			for(sub=0,foundsub=0;sub<subs_out;sub++)
			{
				snd_pcm_info_set_subdevice(info, sub);
				if ((status = snd_ctl_pcm_info(ctl, info)) < 0)
				{
					//printf("cannot get pcm information %d:%d:%d: %s\n", card, device, sub, snd_strerror(status));
					continue;
				}
				//subname = snd_pcm_info_get_subdevice_name(info);
				//printf("Subdevice %d name : %s\n", sub, subname);
				foundsub = 1;
			}
			if (foundsub)
			{
				sprintf(devicename, "hw:%d,%d", card, device);
				sprintf(devicenamedisplayed, "%s %s", shortname, devicename);
				//gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combooutputdev), devicename, shortname);
				gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combooutputdev), devicename, devicenamedisplayed);
				if (!outpreset)
				{
					outpreset = 1;
					g_object_set((gpointer)combooutputdev, "active-id", devicename, NULL);
				}
			}

			snd_pcm_info_free(info);
		}
		while(1);
		//printf("\n");
		count++;
	}
	while (1);
	//printf("%d cards found\n", count);
}

void populate_input_devices_list(GtkWidget *comboinputdev)
{
	int status, count = 0;
	int card = -1;  // use -1 to prime the pump of iterating through card list
	char* longname  = NULL;
	char* shortname = NULL;
	char name[32];
	char devicename[32];
	char devicenamedisplayed[128];
	//const char* subname;
	snd_ctl_t *ctl;
	snd_pcm_info_t *info;
	int device;
	int sub, foundsub, inpreset=0;

	do
	{
		if ((status = snd_card_next(&card)) < 0)
		{
			printf("cannot determine card number: %s\n", snd_strerror(status));
			break;
		}
		if (card<0) break;
		//printf("Card %d:", card);
		if ((status = snd_card_get_name(card, &shortname)) < 0)
		{
			printf("cannot determine card shortname: %s\n", snd_strerror(status));
			break;
		}
		if ((status = snd_card_get_longname(card, &longname)) < 0)
		{
			printf("cannot determine card longname: %s\n", snd_strerror(status));
			break;
		}
		//printf("\tLONG NAME:  %s\n", longname);
		//printf("\tSHORT NAME: %s\n", shortname);

		sprintf(name, "hw:%d", card);
		if ((status = snd_ctl_open(&ctl, name, 0)) < 0)
		{
			printf("cannot open control for card %d: %s\n", card, snd_strerror(status));
			return;
		}

		device = -1;
		do
		{
			status = snd_ctl_pcm_next_device(ctl, &device);
			if (status < 0)
			{
				printf("cannot determine device number: %s\n", snd_strerror(status));
				break;
			}
			if (device<0) break;
			//printf("Device %s,%d\n", name, device);
			snd_pcm_info_malloc(&info);
			snd_pcm_info_set_device(info, (unsigned int)device);

			//snd_pcm_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
			snd_pcm_info_set_stream(info, SND_PCM_STREAM_CAPTURE);
			snd_ctl_pcm_info(ctl, info);
			int subs_in = snd_pcm_info_get_subdevices_count(info);
			//printf("Input subdevices : %d\n", subs_in);
			for(sub=0,foundsub=0;sub<subs_in;sub++)
			{
				snd_pcm_info_set_subdevice(info, sub);
				if ((status = snd_ctl_pcm_info(ctl, info)) < 0)
				{
					//printf("cannot get pcm information %d:%d:%d: %s\n", card, device, sub, snd_strerror(status));
					continue;
				}
				//subname = snd_pcm_info_get_subdevice_name(info);
				//printf("Subdevice %d name : %s\n", sub, subname);
				foundsub = 1;
			}
			if (foundsub)
			{
				sprintf(devicename, "hw:%d,%d", card, device);
				sprintf(devicenamedisplayed, "%s %s", shortname, devicename);
				//gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(comboinputdev), devicename, shortname);
				gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(comboinputdev), devicename, devicenamedisplayed);
				if (!inpreset)
				{
					inpreset = 1;
					g_object_set((gpointer)comboinputdev, "active-id", devicename, NULL);
				}
			}
			snd_pcm_info_free(info);
		}
		while(1);
		//printf("\n");
		count++;
	}
	while (1);
	//printf("%d cards found\n", count);

	strcpy(devicename, "mediafile");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(comboinputdev), devicename, "FFMPEG Media File"); // input file
	if (!inpreset)
	{
		inpreset = 1;
		g_object_set((gpointer)comboinputdev, "active-id", devicename, NULL);
	}
}

void populate_output_devices_list(GtkWidget *combooutputdev)
{
	int status, count = 0;
	int card = -1;  // use -1 to prime the pump of iterating through card list
	char* longname  = NULL;
	char* shortname = NULL;
	char name[32];
	char devicename[32];
	char devicenamedisplayed[128];
	//const char* subname;
	snd_ctl_t *ctl;
	snd_pcm_info_t *info;
	int device;
	int sub, foundsub, outpreset=0;

	do
	{
		if ((status = snd_card_next(&card)) < 0)
		{
			printf("cannot determine card number: %s\n", snd_strerror(status));
			break;
		}
		if (card<0) break;
		//printf("Card %d:", card);
		if ((status = snd_card_get_name(card, &shortname)) < 0)
		{
			printf("cannot determine card shortname: %s\n", snd_strerror(status));
			break;
		}
		if ((status = snd_card_get_longname(card, &longname)) < 0)
		{
			printf("cannot determine card longname: %s\n", snd_strerror(status));
			break;
		}
		//printf("\tLONG NAME:  %s\n", longname);
		//printf("\tSHORT NAME: %s\n", shortname);

		sprintf(name, "hw:%d", card);
		if ((status = snd_ctl_open(&ctl, name, 0)) < 0)
		{
			printf("cannot open control for card %d: %s\n", card, snd_strerror(status));
			return;
		}

		device = -1;
		do
		{
			status = snd_ctl_pcm_next_device(ctl, &device);
			if (status < 0)
			{
				printf("cannot determine device number: %s\n", snd_strerror(status));
				break;
			}
			if (device<0) break;
			//printf("Device %s,%d\n", name, device);
			snd_pcm_info_malloc(&info);
			snd_pcm_info_set_device(info, (unsigned int)device);

			snd_pcm_info_set_stream(info, SND_PCM_STREAM_PLAYBACK);
			snd_ctl_pcm_info(ctl, info);
			int subs_out = snd_pcm_info_get_subdevices_count(info);
			//printf("Output subdevices : %d\n", subs_out);
			for(sub=0,foundsub=0;sub<subs_out;sub++)
			{
				snd_pcm_info_set_subdevice(info, sub);
				if ((status = snd_ctl_pcm_info(ctl, info)) < 0)
				{
					//printf("cannot get pcm information %d:%d:%d: %s\n", card, device, sub, snd_strerror(status));
					continue;
				}
				//subname = snd_pcm_info_get_subdevice_name(info);
				//printf("Subdevice %d name : %s\n", sub, subname);
				foundsub = 1;
			}
			if (foundsub)
			{
				sprintf(devicename, "hw:%d,%d", card, device);
				sprintf(devicenamedisplayed, "%s %s", shortname, devicename);
				//gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combooutputdev), devicename, shortname);
				gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combooutputdev), devicename, devicenamedisplayed);
				if (!outpreset)
				{
					outpreset = 1;
					g_object_set((gpointer)combooutputdev, "active-id", devicename, NULL);
				}
			}

			snd_pcm_info_free(info);
		}
		while(1);
		//printf("\n");
		count++;
	}
	while (1);
	//printf("%d cards found\n", count);

	strcpy(devicename, "pulseaudio");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combooutputdev), devicename, "Pulse Audio"); // output to pulse audio
	if (!outpreset)
	{
		outpreset = 1;
		g_object_set((gpointer)combooutputdev, "active-id", devicename, NULL);
	}
}
