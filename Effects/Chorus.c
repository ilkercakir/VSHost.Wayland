/*
 * Chorus.c
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

#include "Chorus.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */

	strcpy(ae->name, "Chorus");
	audioeffect_allocateparameters(ae, 1);
	audioeffect_initparameter(ae, 0, "Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);

	soundcho *c = ae->data = malloc(sizeof(soundcho));

	soundcho_init(MAXCHORUS, ae->format, ae->rate, ae->channels, c);
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	//soundcho *c = (soundcho *)ae->data;

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
 
	soundcho *c = (soundcho *)ae->data;

	if (ae->parameter[0].value)
		soundcho_add((char*)inbuffer, inbuffersize, c);

/* User defined processing code end */
}

void aef_reinit(audioeffect *ae)
{
/* User defined reinitialization code begin */

	soundcho *c = (soundcho *)ae->data;

	soundcho_close(c);
	soundcho_init(MAXCHORUS, ae->format, ae->rate, ae->channels, c);

//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined reinitialization code end */
}

void aef_close(audioeffect *ae)
{
	audioeffect_deallocateparameters(ae);

/* User defined cleanup code begin  */

	soundcho *c = (soundcho *)ae->data;

	soundcho_close(c);

	free(ae->data);

/* User defined cleanup code end */
}

// Delay Processor in DelayS.c
// Variable Frequency Oscillator Processor in VFO.c

// Chorus

void soundcho_init(int maxcho, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundcho *c)
{
	int i;

	c->format = format;
	c->rate = rate;
	c->channels = channels;
	c->maxcho = maxcho;

	c->prescale = 1.0 / sqrt(c->maxcho+1); // 1/sqrt(N+1);

	float chodelay[MAXCHORUS] = {6.0, 7.0, 8.0}; // delays 6.0 .. 10.0 ms
	float chofreq[MAXCHORUS] = {0.3, 0.4, 0.5}; // modulation frequencies
	float chodepth[MAXCHORUS] = {0.02, 0.02, 0.02}; // modulation depths in percent 0.0 .. 1.0

	for(i=0;i<c->maxcho;i++)
	{
		c->chodelay[i] = chodelay[i];
		c->chofreq[i] = chofreq[i];
		c->chodepth[i] = chodepth[i];
	}

	c->initialized = 0;
	for(i=0;i<c->maxcho;i++)
	{
		sounddelay_init(c->maxcho+1, DLY_LATE, c->chodelay[i], 1.0, c->format, c->rate, c->channels, &(c->d[i]));
		soundvfo_init(c->chofreq[i], c->chodepth[i], c->format, c->rate, c->channels, &(c->v[i]));
		c->dlbuffer[i] = NULL;
	}
}

void soundcho_add(char* inbuffer, int inbuffersize, soundcho *c)
{
	int i, j, k;
	signed short *inshort, *vfshort, *dlshort;

/*
	for(j=0;j<c->maxcho;j++)
	{
		soundvfo_add(inbuffer, inbuffersize, &(c->v[j]));
	}
	inshort = (signed short *)inbuffer;
	for(i=0;i<c->v[0].inbuffersamples;)
		inshort[i++] *= c->prescale;
	for(j=0;j<c->maxcho;j++)
	{
		vfshort = (signed short *)c->v[j].vfobuf;
		for(i=0;i<c->v[j].inbuffersamples;)
		{
			for(k=0;k<c->channels;k++)
				inshort[i++] += vfshort[c->v[j].front++]*c->prescale;
			c->v[j].front %= c->v[j].vfobufsamples;
		}
	}
*/
	if (!c->initialized)
	{
		for(j=0;j<c->maxcho;j++)
			c->dlbuffer[j] = malloc(inbuffersize);
	}

	inshort = (signed short *)inbuffer;
	for(j=0;j<c->maxcho;j++)
	{
		sounddelay_add(inbuffer, inbuffersize, &(c->d[j]));

		dlshort = (signed short *)c->dlbuffer[j];
		for(i=0;i<c->d[j].insamples;)
		{
			dlshort[i++] = c->d[j].fshort[c->d[j].readfront++];
			c->d[j].readfront%=c->d[j].fbuffersamples;
		}
		soundvfo_add(c->dlbuffer[j], inbuffersize, &(c->v[j]));
	}

	for(i=0;i<c->v[0].inbuffersamples;)
		inshort[i++] *= c->prescale;
	for(j=0;j<c->maxcho;j++)
	{
		vfshort = (signed short *)c->v[j].vfobuf;
		for(i=0;i<c->v[j].inbuffersamples;)
		{
			for(k=0;k<c->channels;k++)
				inshort[i++] += vfshort[c->v[j].front++]*c->prescale;
			c->v[j].front %= c->v[j].vfobufsamples;
		}
	}
}

void soundcho_close(soundcho *c)
{
	int i;
	for(i=0;i<c->maxcho;i++)
	{
		soundvfo_close(&(c->v[i]));
		sounddelay_close(&(c->d[i]));
	}

	if (c->initialized)
	{
		for(i=0;i<c->maxcho;i++)
			free(c->dlbuffer[i]);
		c->initialized = 0;
	}
}
