/*
 * Haas.c
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

#include "HaasS.h"

// Haas Effect Processor
void soundhaas_reinit(float millisec, soundhaas *h)
{
	h->millisec = millisec;
	h->initialized = 0;
	sounddelay_reinit(1, DLY_LATE, h->millisec, 1.0, &(h->haasdly));
}

void soundhaas_init(float millisec, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundhaas *h)
{
	h->format = format;
	h->rate = rate;
	h->channels = channels;

	h->millisec = millisec;
	h->initialized = 0;
	sounddelay_init(1, DLY_LATE, h->millisec, 1.0, h->format, h->rate, h->channels, &(h->haasdly));
}

void soundhaas_add(char* inbuffer, int inbuffersize, soundhaas *h)
{
	if (!h->initialized)
	{
		h->physicalwidth = snd_pcm_format_width(h->format); // bits per sample
		h->insamples = inbuffersize / h->physicalwidth * 8;
		h->initialized = 1;
	}
	sounddelay_add(inbuffer, inbuffersize, &(h->haasdly));
	signed short *inshort = (signed short *)inbuffer;
	signed short *fbuffer = h->haasdly.fshort;
	int i;
	for(i=0;i<h->insamples;)
	{
		inshort[i++] *= 0.7; h->haasdly.readfront++; // rescale left channel
		inshort[i++] = fbuffer[h->haasdly.readfront++]; // Haas effect on right channel
		h->haasdly.readfront%=h->haasdly.fbuffersamples;
	}
}

void soundhaas_close(soundhaas *h)
{
	sounddelay_close(&(h->haasdly));
	h->initialized = 0;
}
