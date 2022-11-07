/*
 * Tremolo.c
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

#include "Tremolo.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */

	strcpy(ae->name, "Tremolo");
	audioeffect_allocateparameters(ae, 4);
	audioeffect_initparameter(ae, 0, "Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae, 1, "Rate Hz", 1.0, 8.0, 3.0, 0.1, 1, pt_scale);
	audioeffect_initparameter(ae, 2, "Depth", 0.1, 1.0, 0.2, 0.1, 1, pt_scale);
	audioeffect_initparameter(ae, 3, "Invert", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);

	soundtremolo *t = ae->data = malloc(sizeof(soundtremolo));

	soundtremolo_init((int)ae->parameter[0].value, ae->parameter[1].value, ae->parameter[2].value, ae->parameter[3].value, ae->format, ae->rate, ae->channels, t);
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	soundtremolo *t = (soundtremolo *)ae->data;

	switch(i)
	{
		case 0: // Enable
			t->enabled = (int)ae->parameter[i].value;
			break;
		case 1: // Tremolo rate
			t->tremolorate = ae->parameter[i].value;
			break;
		case 2: // Depth
			t->depth = ae->parameter[i].value;
			break;
	}
//printf("aef_setparameter %d = %2.2f\n", i, ae->parameter[i].value);

/* User defined parameter setter code end */
}

float aef_getparameter(audioeffect *ae, int i)
{
	return ae->parameter[i].value;
}

void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize)
{
/* User defined processing code begin */
 
	soundtremolo *t = (soundtremolo *)ae->data;

	if (ae->parameter[0].value)
		soundtremolo_add((char*)inbuffer, inbuffersize, t);

/* User defined processing code end */
}

void aef_reinit(audioeffect *ae)
{
/* User defined reinitialization code begin */

	soundtremolo *t = (soundtremolo *)ae->data;

	soundtremolo_reinit((int)ae->parameter[0].value, ae->parameter[1].value, ae->parameter[2].value, ae->parameter[3].value, t);
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined reinitialization code end */
}

void aef_close(audioeffect *ae)
{
	audioeffect_deallocateparameters(ae);

/* User defined cleanup code begin  */

	soundtremolo *t = (soundtremolo *)ae->data;

	soundtremolo_close(t);

	free(ae->data);

/* User defined cleanup code end */
}

// Tremolo Effect Processor

void soundtremolo_reinit(int enabled, float tremolorate, float depth, int invert, soundtremolo *t)
{
	soundtremolo_init(enabled, tremolorate, depth, invert, t->format, t->rate, t->channels, t);
}

void soundtremolo_init(int enabled, float tremolorate, float depth, int invert, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundtremolo *t)
{
	t->format = format;
	t->rate = rate;
	t->channels = channels;
	t->depth = depth;
	t->tremolorate = tremolorate;
	t->invert = invert;
	t->enabled = enabled;
	t->initialized = 0;
	t->framecount = 0;
}

void soundtremolo_add(char* inbuffer, int inbuffersize, soundtremolo *t)
{
	if (t->enabled)
	{
		if (!t->initialized)
		{
			t->initialized = 1;
			int physicalwidth = snd_pcm_format_width(t->format); // bits per sample
			int insamples = inbuffersize / physicalwidth * 8;
			t->frames = insamples / t->channels;
			t->framesinT = (int)((float)t->rate / (float)t->tremolorate);
		}
		signed short *inshort = (signed short *)inbuffer;
		int i,j;
		for(i=0,j=0;i<=t->frames;i++,t->framecount++)
		{
			float time = ((float)t->framecount / (float)t->rate);
			float theta = 2.0*M_PI*t->tremolorate*time; // w*t = 2*pi*f*t
			int k;
			for(k=0;k<t->channels;k++)
			{
				//inshort[j++] *= 1.0-((t->depth/2.0)*(1.0-sin(theta)));
				float inverter = (t->invert?-2.0*(k%2)+1.0:1.0); // y=-2x+1
				inshort[j++] *= 1.0-((t->depth/2.0)*(1.0-sin(inverter*theta)));
			}
			
		}
		t->framecount %= t->framesinT;
	}
}

void soundtremolo_close(soundtremolo *t)
{
}

