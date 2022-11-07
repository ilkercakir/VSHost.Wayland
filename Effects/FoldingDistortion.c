/*
 * FoldingDistortion.c
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

#include "FoldingDistortion.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */

	strcpy(ae->name, "Folding Distortion");
	audioeffect_allocateparameters(ae, 3);
	audioeffect_initparameter(ae, 0, "Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae, 1, "Threshold", 1.0, 32000.0, 2000.0, 100.0, 0, pt_scale);
	audioeffect_initparameter(ae, 2, "Gain", 0.1, 4.0, 1.0, 0.1, 0, pt_scale);

	soundfoldingdistortion *d = ae->data = malloc(sizeof(soundfoldingdistortion));

	soundfoldingdistort_init(ae->parameter[1].value, ae->parameter[2].value, ae->format, ae->rate, ae->channels, d);
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	soundfoldingdistortion *d = (soundfoldingdistortion *)ae->data;

	switch(i)
	{
		case 0: // Enable
			break;
		case 1: // Threshold
			d->threshold = ae->parameter[i].value;
			break;
		case 2: // Gain
			d->gain = ae->parameter[i].value;
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
 
	soundfoldingdistortion *d = (soundfoldingdistortion *)ae->data;

	if (ae->parameter[0].value)
		soundfoldingdistort_add((char*)inbuffer, inbuffersize, d);

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

	soundfoldingdistortion *d = (soundfoldingdistortion *)ae->data;

	soundfoldingdistort_close(d);

	free(ae->data);

/* User defined cleanup code end */
}

// Folding Distortion

void soundfoldingdistort_init(float threshold, float gain, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundfoldingdistortion *d)
{
	d->format = format;
	d->rate = rate;
	d->channels = channels;
	d->threshold = threshold;
	d->gain = gain;

	d->initialized = 0;
}

void soundfoldingdistort_add(char* inbuffer, int inbuffersize, soundfoldingdistortion *d)
{
	if (!d->initialized)
	{
		d->physicalwidth = snd_pcm_format_width(d->format); // bits per sample
		d->insamples = inbuffersize / d->physicalwidth * 8;
		d->initialized = 1;
	}
	signed short *inshort = (signed short *)inbuffer;
	int i;
	for(i=0;i<d->insamples;i++)
		inshort[i]= (inshort[i]>d->threshold?2*d->threshold-inshort[i]:(inshort[i]<-d->threshold?-2*d->threshold-inshort[i]:inshort[i]))*d->gain;
		//inshort[i] = (inshort[i]>d->threshold?d->threshold:(inshort[i]<-d->threshold?-d->threshold:inshort[i]))*d->gain;
}

void soundfoldingdistort_close(soundfoldingdistortion *d)
{
	d->initialized = 0;
}
