/*
 * Stereoizer.c
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

#include "Stereoizer.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */

	soundstereo *s = ae->data = malloc(sizeof(soundstereo));

	strcpy(ae->name, "Stereo");
	audioeffect_allocateparameters(ae, 7);
	audioeffect_initparameter(ae,  0, "Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae,  1, "Delay", 1.0, 30.0, 15.0, 1.0, 1, pt_scale);
	audioeffect_initparameter(ae,  2, "Modulation", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae,  3, "L Rate Hz", 0.1, 2.0, 0.8, 0.1, 1, pt_scale);
	audioeffect_initparameter(ae,  4, "L Depth %", 0.1, 5.0, 1.7, 0.1, 1, pt_scale);
	audioeffect_initparameter(ae,  5, "R Rate Hz", 0.1, 2.0, 1.2, 0.1, 1, pt_scale);
	audioeffect_initparameter(ae,  6, "R Depth %", 0.1, 5.0, 0.9, 0.1, 1, pt_scale);

	soundstereo_init(ae->format, ae->rate, ae->channels, s);
	soundvfo_init(ae->parameter[3].value, ae->parameter[4].value/100.0, ae->format, ae->rate, 1, &(s->v[0])); // L
	soundvfo_init(ae->parameter[5].value, ae->parameter[6].value/100.0, ae->format, ae->rate, 1, &(s->v[1])); // R
	soundhaas_init(ae->parameter[1].value, ae->format, ae->rate, ae->channels, &(s->h));
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	switch(i)
	{
		case 0: // Enable
			break;
		case 1:
			break;
		case 2: // Enable
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 6:
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
 
	soundstereo *s = (soundstereo *)ae->data;

	if (ae->parameter[0].value)
		soundstereo_addhaas((char*)inbuffer, inbuffersize, s);
	if (ae->parameter[2].value)
		soundstereo_addmodulation((char*)inbuffer, inbuffersize, s);

/* User defined processing code end */
}

void aef_reinit(audioeffect *ae)
{
/* User defined reinitialization code begin */

	soundstereo *s = (soundstereo *)ae->data;

	soundvfo_close(&(s->v[0]));
	soundvfo_close(&(s->v[1]));
	soundstereo_close(s);
	soundstereo_init(ae->format, ae->rate, ae->channels, s);
	soundvfo_init(ae->parameter[3].value, ae->parameter[4].value/100.0, ae->format, ae->rate, 1, &(s->v[0])); // L
	soundvfo_init(ae->parameter[5].value, ae->parameter[6].value/100.0, ae->format, ae->rate, 1, &(s->v[1])); // R

	soundhaas_reinit(ae->parameter[1].value, &(s->h));
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined reinitialization code end */
}

void aef_close(audioeffect *ae)
{
	audioeffect_deallocateparameters(ae);

/* User defined cleanup code begin  */

	soundstereo *s = (soundstereo *)ae->data;

	soundhaas_close(&(s->h));
	soundvfo_close(&(s->v[0]));
	soundvfo_close(&(s->v[1]));
	soundstereo_close(s);

	free(ae->data);

/* User defined cleanup code end */
}

// Haas Effect Processor in HaasS.c, Variable Frequency Oscillator Processor in VFO.c

// Stereoizer

void soundstereo_init(snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundstereo *s)
{
	s->format = format;
	s->rate = rate;
	s->channels = channels;
	s->buffer = NULL;

	s->v = malloc(s->channels * sizeof(soundvfo));
}

void soundstereo_split(char* inbuffer, int inbuffersize, soundstereo *s)
{
	int i,j;
	signed short *inshort = (signed short *)inbuffer;
	signed short *bufshort;

	if (!s->buffer)
	{
		s->physicalwidth = snd_pcm_format_width(s->format) / 8;
		s->insamples = inbuffersize / s->physicalwidth;
		s->inframes = s->insamples / s->channels;
		s->buffer = malloc(s->channels * sizeof(char*));
		for(i=0;i<s->channels;i++)
			s->buffer[i] = malloc(s->inframes*s->physicalwidth);
	}

	for(j=0;j<s->inframes;j++)
	{
		for(i=0;i<s->channels;i++)
		{
			bufshort = (signed short *)s->buffer[i];
			bufshort[j] = inshort[s->channels*j+i];
		}
	}
}

void soundstereo_merge(char* outbuffer, soundstereo *s)
{
	int i,j;
	signed short *outshort = (signed short *)outbuffer;
	signed short *bufshort;

	for(j=0;j<s->inframes;j++)
	{
		for(i=0;i<s->channels;i++)
		{
			bufshort = (signed short *)s->buffer[i];
			outshort[s->channels*j+i] = bufshort[j];
		}
	}
}

void soundstereo_addhaas(char* inbuffer, int inbuffersize, soundstereo *s)
{
	soundhaas_add((char*)inbuffer, inbuffersize, &(s->h));
}

void soundstereo_addmodulation(char* inbuffer, int inbuffersize, soundstereo *s)
{
	soundstereo_split(inbuffer, inbuffersize, s);

	soundvfo_add(s->buffer[0], s->inframes*s->physicalwidth, &(s->v[0]));
	soundvfo_add(s->buffer[1], s->inframes*s->physicalwidth, &(s->v[1]));

	int i, j;
	signed short *inshort = (signed short *)inbuffer;
	for(j=0;j<s->inframes;j++)
	{
		for(i=0;i<s->channels;i++)
		{
			signed short *vfoshort = (signed short *)s->v[i].vfobuf;
			inshort[j*s->channels+i] = vfoshort[s->v[i].front++];
			s->v[i].front %= s->v[i].vfobufsamples;
		}
	}
}

void soundstereo_close(soundstereo *s)
{
	int i;
	if (s->buffer)
	{
		for(i=0;i<s->channels;i++)
			free(s->buffer[i]);
		free(s->buffer);
		s->buffer = NULL;
	}
	free(s->v);
}
