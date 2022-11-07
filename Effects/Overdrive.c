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

#include "Overdrive.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */
	soundoverdrive *o = ae->data = malloc(sizeof(soundoverdrive));
	set_overdriveeq(&(o->odeqdef));

	strcpy(ae->name, "Overdrive");
	audioeffect_allocateparameters(ae, 5);
	audioeffect_initparameter(ae, 0, "Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae, 1, "Drive", 10.0, 200.0, 50.0, 1.0, 0, pt_scale);
	audioeffect_initparameter(ae, 2, "Level", 0.1, 4.0, 1.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae, 3, "EQ Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae, 4, o->odeqdef.eqlabels[0], 2000.0, 10000.0, o->odeqdef.eqfreqs[0], 100.0, 1, pt_scale);

	soundoverdrive_init(ae->parameter[1].value, ae->parameter[2].value, (int)ae->parameter[3].value, &(o->odeqdef), ae->format, ae->rate, ae->channels, o);

//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	soundoverdrive *o = (soundoverdrive *)ae->data;

	switch(i)
	{
		case 0: // Enable
			break;
		case 1: // Drive
			o->a = ae->parameter[i].value;
			break;
		case 2: // Level
			o->l = ae->parameter[i].value;
			break;
		case 3: // EQ Enable
			break;
		case 4: // EQ Band 0
			o->odeqdef.eqfreqs[0] = ae->parameter[i].value;
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
 
	soundoverdrive *o = (soundoverdrive *)ae->data;

	if (ae->parameter[0].value)
		soundoverdrive_add((char*)inbuffer, inbuffersize, o);

/* User defined processing code end */
}

void aef_reinit(audioeffect *ae)
{
/* User defined reinitialization code begin */

	soundoverdrive *o = (soundoverdrive *)ae->data;

	soundoverdrive_reinit(ae->parameter[1].value, ae->parameter[2].value, (int)ae->parameter[3].value, &(o->odeqdef), o);
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined reinitialization code end */
}

void aef_close(audioeffect *ae)
{
	audioeffect_deallocateparameters(ae);

/* User defined cleanup code begin  */

	soundoverdrive *o = (soundoverdrive *)ae->data;

	soundoverdrive_close(o);

	free(ae->data);

/* User defined cleanup code end */
}

// Overdrive

void set_overdriveeq(eqdefaults *d)
{
	float default_eqfreqs[1] = {6000.0};
	char* default_eqlabels[1] = {"HSH"};
	filtertype default_filtertypes[] = {HSH};
	float default_dbGains[1] = {-12.0};

	int i;
	for(i=0;i<1;i++)
	{
		d->eqfreqs[i] = default_eqfreqs[i];
		strcpy(d->eqlabels[i], default_eqlabels[i]);
		d->filtertypes[i] = default_filtertypes[i];
		d->dbGains[i] = default_dbGains[i];
	}
}

void soundoverdrive_set(float drive, float level, soundoverdrive *o)
{
	o->a = drive;
	o->l = level;
	o->dnf = (float)((0x1<<(snd_pcm_format_width(o->format)-1))-1); // maxint/2
	o->nf = 1.0 / o->dnf;
}

void soundoverdrive_reinit(float drive, float level, int eqenabled, eqdefaults *d, soundoverdrive *o)
{
	AudioEqualizer_close(&(o->odeq));

	soundoverdrive_set(drive, level, o);

	AudioEqualizer_init(&(o->odeq), 1, 1.0, eqenabled, 0, o->format, o->rate, o->channels, d);
}

void soundoverdrive_init(float drive, float level, int eqenabled, eqdefaults *d, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundoverdrive *o)
{
	o->format = format;
	o->rate = rate;
	o->channels = channels;
	soundoverdrive_set(drive, level, o);

	AudioEqualizer_init(&(o->odeq), 1, 1.0, eqenabled, 0, o->format, o->rate, o->channels, d);

	o->initialized = 0;

	//printf("\t%f\t%f\t%f\t%f\n", o->a, o->denom, o->nf, o->dnf);
}

void soundoverdrive_add(char* inbuffer, int inbuffersize, soundoverdrive *o)
{
	if (!o->initialized)
	{
		o->physicalwidth = snd_pcm_format_width(o->format); // bits per sample
		o->insamples = inbuffersize / o->physicalwidth * 8;
		o->initialized = 1;
	}
	signed short *inshort = (signed short *)inbuffer;
	int i;
	for(i=0;i<o->insamples;i++)
	{
		float x = ((float)inshort[i]) * o->nf;
		float y = (inshort[i]>=0?1.0-exp(-o->a*x):exp(o->a*x)-1.0) * o->l * log(o->a) / o->a;
		inshort[i] = (signed short)(y * o->dnf);
	}
	AudioEqualizer_BiQuadProcess(&(o->odeq), (uint8_t*)inbuffer, inbuffersize);
}

void soundoverdrive_close(soundoverdrive *o)
{
	AudioEqualizer_close(&(o->odeq));
	o->initialized = 0;
}

