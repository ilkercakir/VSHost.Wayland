/*
 * Mono.c
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

#include "Mono.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */

	strcpy(ae->name, "Mono");
	audioeffect_allocateparameters(ae, 1);
	audioeffect_initparameter(ae, 0, "Enable", 0.0, 1.0, 0.0, 1.0, 0, pt_switch);

	soundmono *m = ae->data = malloc(sizeof(soundmono));

	soundmono_init(ae->format, ae->rate, ae->channels, m);
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	//soundmono *m = (soundmono *)ae->data;

	switch(i)
	{
		case 0: // Enable
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
 
	soundmono *m = (soundmono *)ae->data;

	if (ae->parameter[0].value)
		soundmono_add((char*)inbuffer, inbuffersize, m);

/* User defined processing code end */
}

void aef_reinit(audioeffect *ae)
{
/* User defined reinitialization code begin */

//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined reinitialization code end */
}

void aef_close(audioeffect *ae)
{
	audioeffect_deallocateparameters(ae);

/* User defined cleanup code begin  */

	soundmono *m = (soundmono *)ae->data;

	soundmono_close(m);

	free(ae->data);

/* User defined cleanup code end */
}

// Mono

void soundmono_init(snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundmono *m)
{
	m->format = format;
	m->rate = rate;
	m->channels = channels;

	m->initialized = 0;
	m->prescale = 1.0/sqrt(m->channels);
}

void soundmono_add(char* inbuffer, int inbuffersize, soundmono *m)
{
	if (!m->initialized)
	{
		m->physicalwidth = snd_pcm_format_width(m->format); // bits per sample
		m->insamples = inbuffersize / m->physicalwidth * 8;
		m->initialized = 1;
		//printf("init %d %d %d %d\n", inbuffersize, m->format, m->physicalwidth, m->insamples);
	}
	signed short *inshort = (signed short *)inbuffer;
	int i,j;
	for(i=0;i<m->insamples;)
	{
		signed short value = 0;
		for(j=0;j<m->channels;j++) value+=inshort[i+j]*m->prescale;
		for(j=0;j<m->channels;j++) inshort[i++]=value;
	}
}

void soundmono_close(soundmono *m)
{
	m->initialized = 0;
}
