/*
 * DelayS.c
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

#include "DelayS.h"

// Delay Effect Processor

extern inline signed short sounddelay_readsample(sounddelay *s);

void sounddelay_prescale(sounddelay *s)
{
		switch (s->delaytype)
	{
		case DLY_ECHO: 
			s->prescale = sqrt(1.0 - s->feedback*s->feedback); // prescale=sqrt(sum(r^2n)), n=0..infinite
			break;
		case DLY_DELAY:
			s->prescale = 1.0 / sqrt(1.0 + s->feedback*s->feedback); // prescale = 1/sqrt(1 + r^2)
			break;
		case DLY_REVERB:
			s->prescale = sqrt((1.0 - s->feedback*s->feedback)/(((float)(s->N-1))*s->feedback*s->feedback + 1.0)); // prescale=sqrt(sum(r^2n)-1), for all channels, n=0..infinite
			break;
		case DLY_LATE: // Single delayed signal, no original
			s->prescale = 1.0;
			break;
	}
}

void sounddelay_reinit(int N, dly_type delaytype, float millisec, float feedback, sounddelay *s)
{
	if (s->fbuffer)
	{
		free(s->fbuffer);
		s->fbuffer = NULL;
	}

	s->N = N;
	s->delaytype = delaytype;
	s->feedback = feedback;
	s->millisec = millisec;

	sounddelay_prescale(s);
}

void sounddelay_init(int N, dly_type delaytype, float millisec, float feedback, snd_pcm_format_t format, unsigned int rate, unsigned int channels, sounddelay *s)
{
	s->format = format;
	s->rate = rate;
	s->channels = channels;
	s->N = N;
	s->delaytype = delaytype;
	s->feedback = feedback;
	s->millisec = millisec;
	s->fbuffer = NULL;

	sounddelay_prescale(s);
	//printf("Delay initialized, type %d, %5.2f ms, %5.2f feedback, %d rate, %d channels\n", s->delaytype, s->millisec, s->feedback, s->rate, s->channels);
}

void sounddelay_add(char* inbuffer, int inbuffersize, sounddelay *s)
{
	int i;
	signed short *inshort;

	if (!s->fbuffer)
	{
		s->physicalwidth = snd_pcm_format_width(s->format);
		s->insamples = inbuffersize * 8 / s->physicalwidth;
		s->delaysamples = ceil((s->millisec / 1000.0) * (float)s->rate) * s->channels;
		s->delaybytes = s->delaysamples * s->physicalwidth / 8;

		s->fbuffersize = s->delaybytes + inbuffersize;
		s->fbuffer = malloc(s->fbuffersize);
		memset(s->fbuffer, 0, s->fbuffersize);
		s->fbuffersamples = s->insamples + s->delaysamples;
		s->fshort = (signed short *)s->fbuffer;

		s->rear = s->delaysamples;
		s->front = 0;
		s->readfront = 0;
	}
	inshort = (signed short *)inbuffer;

	switch (s->delaytype)
	{
		case DLY_ECHO: // Repeating echo added to original
			for(i=0; i<s->insamples; i++)
			{
				inshort[i]*=s->prescale;
				s->fshort[s->rear++] = inshort[i] += s->fshort[s->front++]*s->feedback;
				s->front%=s->fbuffersamples;
				s->rear%=s->fbuffersamples;
			}
			break;
		case DLY_DELAY: // Single delayed signal added to original
			for(i=0;i<s->insamples; i++)
			{
				inshort[i]*=s->prescale;
				s->fshort[s->rear++] = inshort[i];
				inshort[i] += s->fshort[s->front++]*s->feedback;
				s->front%=s->fbuffersamples;
				s->rear%=s->fbuffersamples;
			}
			break;
		case DLY_REVERB: // Only repeating echo, no original
			for(i=0; i<s->insamples; i++)
			{
				//s->fshort[s->rear++] = inshort[i]*prescale + s->fshort[s->front++]*s->feedback;
				s->fshort[s->rear++] = (inshort[i]*s->prescale + s->fshort[s->front++])*s->feedback;
				s->front%=s->fbuffersamples;
				s->rear%=s->fbuffersamples;
			}
			break;
		case DLY_LATE: // Single delayed signal, no original
			for(i=0;i<s->insamples; i++)
			{
				s->fshort[s->rear++] = inshort[i]*s->feedback;
				s->front++;
				s->front%=s->fbuffersamples;
				s->rear%=s->fbuffersamples;
			}
			break;
		default:
			break;
	}
}

void sounddelay_close(sounddelay *s)
{
	if (s->fbuffer)
	{
		free(s->fbuffer);
		s->fbuffer = NULL;
	}
}
