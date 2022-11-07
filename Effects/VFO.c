/*
 * VFO.c
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

#include "VFO.h"

void soundvfo_init(float vfofreq, float vfodepth, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundvfo *v)
{
	v->format = format;
	v->rate = rate;
	v->channels = channels;
	v->vfofreq = vfofreq;
	v->vfodepth = vfodepth;

	v->vfobuf = NULL;
	v->rear = v->front = 0;	
}

void soundvfo_add(char* inbuffer, int inbuffersize, soundvfo *v)
{
	int i, j, delta, index;
	float frac, ix;

	if (!v->vfobuf)
	{
		v->physicalwidth = snd_pcm_format_width(v->format);
		v->framebytes = v->physicalwidth / 8 * v->channels;
		v->inbufferframes = inbuffersize / v->framebytes;
		v->inbuffersamples = v->inbufferframes * v->channels;
		v->framesinT = (int)((float)v->rate / v->vfofreq);
		v->periods = v->framesinT / v->inbufferframes;
		v->periods = (v->periods>>2)<<2; // Multiple of 4
		v->period = 0;

		v->lastframe = malloc(v->channels*v->physicalwidth);
		signed short *lf = (signed short *)v->lastframe;
		for(j=0;j<v->channels;j++)
			lf[j] = 0;

		v->peak = v->inbufferframes * v->vfodepth;
		v->vfobufframes = v->inbufferframes;
		for(i=0;i<v->periods>>1;i++) // simulate half modulation period
		{
			delta = (int)(((float)v->peak)*sin((float)i*2.0*M_PI/(float)v->periods));
			v->vfobufframes += delta;
		}
		//printf("peak %d periods %d vfobufframes %d\n", v->peak, v->periods, v->vfobufframes);
		v->vfobufsamples = v->vfobufframes * v->channels;
		v->vfobuf = malloc(v->vfobufsamples*v->physicalwidth);

		v->L = v->inbufferframes;
		//printf("channels %d\n", v->channels);
	}

	signed short *inshort, *vfshort;
	inshort = (signed short *)inbuffer;
	vfshort = (signed short *)v->vfobuf;

	if (v->period<v->periods>>1)
		delta = (int)(((float)v->peak)*sin((float)v->period*2.0*M_PI/(float)v->periods));
	else
		delta = -(int)(((float)v->peak)*sin((float)(v->period-(v->periods>>1))*2.0*M_PI/(float)v->periods));

	v->M = v->L + delta;
//printf("M %d L %d ", v->M, v->L);
	if (v->M>v->L) // interpolation
	{
		//printf("interpolating period %d\n", v->period);
		frac = (float)v->L/(float)v->M;
		for(j=0;j<v->channels;j++) // frame 1
		{
			vfshort[v->rear++] = ((float)inshort[j])*frac + ((float)v->lastframe[j])*(1.0-frac);
		}
		v->rear %= v->vfobufsamples;
		for(i=2;i<v->M;i++) // frame 2 .. M-1
		{
			ix = (float)(i*v->L)/(float)v->M;
			index = (int)ix; frac = ix - (float)index;
			for(j=0;j<v->channels;j++)
			{
				vfshort[v->rear++] = ((float)inshort[index*v->channels+j])*frac + ((float)inshort[(index-1)*v->channels+j])*(1.0-frac);
			}
			v->rear %= v->vfobufsamples;
		}
		for(j=0;j<v->channels;j++) // frame M
		{
			v->lastframe[j] = vfshort[v->rear++] = (float)inshort[(v->L-1)*v->channels+j];
		}
		v->rear %= v->vfobufsamples;
	}
	else if (v->M<v->L) // decimation
	{
		//printf("decimating period %d\n", v->period);
		for(i=1;i<v->M;i++) // frame 1 .. M-1
		{
			ix = (float)(i*v->L)/(float)v->M;
			index = (int)ix; frac = ix - (float)index;
			for(j=0;j<v->channels;j++)
			{
				vfshort[v->rear++] = ((float)inshort[(index-1)*v->channels+j])*(1.0-frac) + ((float)inshort[index*v->channels+j])*frac;
			}
			v->rear %= v->vfobufsamples;
		}
		for(j=0;j<v->channels;j++) // frame M
		{
			v->lastframe[j] = vfshort[v->rear++] = (float)inshort[(v->L-1)*v->channels+j];
		}
		v->rear %= v->vfobufsamples;
	}
	else // copy
	{
		//printf("copying period %d\n", v->period); 
		for(i=0;i<v->inbufferframes;i++)
		{
			for(j=0;j<v->channels;j++)
				vfshort[v->rear++] = inshort[i*v->channels+j];
			v->rear %= v->vfobufsamples;
		}
		for(j=0;j<v->channels;j++) // frame L
		{
			v->lastframe[j] = (float)inshort[(v->L-1)*v->channels+j];
		}
	}

	v->period++;
	v->period %= v->periods;
}

void soundvfo_close(soundvfo *v)
{
	if (v->vfobuf)
	{
		free(v->vfobuf);
		v->vfobuf = NULL;
		free(v->lastframe);
		v->lastframe = NULL;
	}
}
