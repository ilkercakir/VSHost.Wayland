/*
 * Gain.c
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

#include "Gain.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */

	strcpy(ae->name, "Gain");
	audioeffect_allocateparameters(ae, 2);
	audioeffect_initparameter(ae, 0, "Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae, 1, "Gain", 0.0, 3.0, 1.0, 0.1, 0, pt_scale);

	gain *gaindata = ae->data = malloc(sizeof(gain));
	soundgain *g = &(gaindata->g);

	gaindata->format = ae->format;
	gaindata->rate = ae->rate;
	gaindata->channels = ae->channels;

	soundgain_init(ae->parameter[1].minval, ae->parameter[1].maxval, ae->parameter[1].value, gaindata->format, gaindata->rate, gaindata->channels, g);
//printf("%f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	gain *gaindata = (gain *)ae->data;
	soundgain *g = &(gaindata->g);

	switch(i)
	{
		case 0: // Enable
			break;
		case 1: // Gain
			g->gainvalue = ae->parameter[i].value;			
			break;
		default:
			return;
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

	gain *gaindata = (gain *)ae->data;
	soundgain *g = &(gaindata->g);

	if (ae->parameter[0].value)
		soundgain_add((char*)inbuffer, inbuffersize, g);

/* User defined processing code end */
}

void aef_reinit(audioeffect *ae)
{
/* User defined reinitialization code begin */

	gain *gaindata = (gain *)ae->data;
	soundgain *g = &(gaindata->g);

	soundgain_reinit(ae->parameter[1].minval, ae->parameter[1].maxval, ae->parameter[1].value, g);

/* User defined reinitialization code end */
}

void aef_close(audioeffect *ae)
{
	audioeffect_deallocateparameters(ae);

/* User defined cleanup code begin */
 
	free(ae->data);

/* User defined cleanup code end */
}

// Gain Processor
void soundgain_reinit(float mingain, float maxgain, float gainvalue, soundgain *g)
{
	g->mingain = mingain;
	g->maxgain = maxgain;
	g->gainvalue = gainvalue;
	g->initialized = 0;
}

void soundgain_init(float mingain, float maxgain, float gainvalue, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundgain *g)
{
	g->format = format;
	g->rate = rate;
	g->channels = channels;
	g->mingain = mingain;
	g->maxgain = maxgain;
	g->gainvalue = gainvalue;
	g->initialized = 0;
	//printf("Delay initialized, type %d, %5.2f ms, %5.2f feedback, %d rate, %d channels\n", s->delaytype, s->millisec, s->feedback, s->rate, s->channels);
}

void soundgain_add(char* inbuffer, int inbuffersize, soundgain *g)
{
	int i;
	signed short *inshort;

	inshort = (signed short *)inbuffer;
	if (!g->initialized)
	{
		g->physicalwidth = snd_pcm_format_width(g->format);
		g->insamples = inbuffersize * 8 / g->physicalwidth;
		g->initialized = 1;
	}
	for(i=0;i<g->insamples;i++)
		inshort[i] *= g->gainvalue;
}

void soundgain_close(soundgain *s)
{
}
