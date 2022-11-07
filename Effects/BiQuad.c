/*
 * BiQuad.c
 * 
 * Copyright 2017  <pi@raspberrypi>
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

#include "BiQuad.h"

extern inline float BiQuad(biquad *b, float sample);

/* sets up a BiQuad Filter */
void BiQuad_init(biquad *b, filtertype type, float dbGain, float freq, float srate, float bandwidth)
{
	float A, omega, sn, cs, alpha, beta;
	float a0, a1, a2, b0, b1, b2;

	b->type = type;
	b->dbGain = dbGain;
	b->freq = freq;
	b->srate = srate;
	b->bandwidth = bandwidth;

   /* setup variables */
   A = pow(10, dbGain /40);
   omega = 2 * M_PI * freq / srate;
   sn = sin(omega);
   cs = cos(omega);
   alpha = sn * sinh(M_LN2 / 2 * bandwidth * omega / sn);
   beta = sqrt(A + A);

   switch (type) {
   case LPF:
       b0 = (1 - cs) /2;
       b1 = 1 - cs;
       b2 = (1 - cs) /2;
       a0 = 1 + alpha;
       a1 = -2 * cs;
       a2 = 1 - alpha;
       break;
   case HPF:
       b0 = (1 + cs) /2;
       b1 = -(1 + cs);
       b2 = (1 + cs) /2;
       a0 = 1 + alpha;
       a1 = -2 * cs;
       a2 = 1 - alpha;
       break;
   case BPF:
       b0 = alpha;
       b1 = 0;
       b2 = -alpha;
       a0 = 1 + alpha;
       a1 = -2 * cs;
       a2 = 1 - alpha;
       break;
   case NOTCH:
       b0 = 1;
       b1 = -2 * cs;
       b2 = 1;
       a0 = 1 + alpha;
       a1 = -2 * cs;
       a2 = 1 - alpha;
       break;
   case PEQ:
       b0 = 1 + (alpha * A);
       b1 = -2 * cs;
       b2 = 1 - (alpha * A);
       a0 = 1 + (alpha /A);
       a1 = -2 * cs;
       a2 = 1 - (alpha /A);
       break;
   case LSH:
       b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
       b1 = 2 * A * ((A - 1) - (A + 1) * cs);
       b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
       a0 = (A + 1) + (A - 1) * cs + beta * sn;
       a1 = -2 * ((A - 1) + (A + 1) * cs);
       a2 = (A + 1) + (A - 1) * cs - beta * sn;
       break;
   case HSH:
       b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
       b1 = -2 * A * ((A - 1) + (A + 1) * cs);
       b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
       a0 = (A + 1) - (A - 1) * cs + beta * sn;
       a1 = 2 * ((A - 1) - (A + 1) * cs);
       a2 = (A + 1) - (A - 1) * cs - beta * sn;
       break;
   }

   /* precompute the coefficients */
   b->a0 = b0 /a0;
   b->a1 = b1 /a0;
   b->a2 = b2 /a0;
   b->a3 = a1 /a0;
   b->a4 = a2 /a0;

   /* zero initial samples */
   b->x1 = b->x2 = 0;
   b->y1 = b->y2 = 0;
}

void BiQuad_close(biquad *b)
{
}

void AudioEqualizer_calcEffectiveGain(audioequalizer *eq)
{
	int i;

	for(i=0, eq->effgain = 0.0;i<eq->bands;i++)
	{
		eq->effgain += pow(10, eq->dbGains[i]/5.0);
	}
	eq->effgain = 1.0 / sqrt(eq->effgain/eq->bands);
	eq->effgain *= 0.9; // 10% margin
//printf("EQ %5.2f\n", eq->effgain);
}

void AudioEqualizer_init(audioequalizer *eq, int eqbands, float eqoctave, int eqenabled, int eqautoleveling, snd_pcm_format_t format, unsigned int rate, unsigned int channels, eqdefaults *eqd)
{
	eq->bands = eqbands;
	eq->octave = eqoctave;
	eq->enabled = eqenabled;
	eq->autoleveling = eqautoleveling;
	eq->eqmutex = malloc(sizeof(pthread_mutex_t));
	int ret;
	if((ret=pthread_mutex_init(eq->eqmutex, NULL))!=0 )
		printf("mutex init failed, %d\n", ret);

	eq->format = format;
	eq->rate = rate;
	eq->channels = channels;
	eq->bq = malloc(eqbands*channels*sizeof(biquad));
	eq->freqs = malloc(eqbands*sizeof(float));
	eq->labels = malloc(eqbands*sizeof(char*));
	eq->filtertypes = malloc(eqbands*sizeof(filtertype*));
	eq->dbGains = malloc(eqbands*sizeof(float*));

	int i, j;
	for(i=0;i<eq->bands;i++)
	{
		eq->freqs[i] = eqd->eqfreqs[i];
		eq->labels[i] = malloc(20*sizeof(char));
		strcpy(eq->labels[i], eqd->eqlabels[i]);
		eq->filtertypes[i] = eqd->filtertypes[i];
		eq->dbGains[i] = eqd->dbGains[i];
		for(j=0;j<eq->channels;j++)
			BiQuad_init(&(eq->bq[i*eq->channels+j]), eq->filtertypes[i], eqd->dbGains[i], eq->freqs[i], eq->rate, eq->octave);
	}
	if (eq->autoleveling)
	{
		AudioEqualizer_calcEffectiveGain(eq);
	}
	else
		eq->effgain = 1.0;

	eq->volume = 1.0;
}

void AudioEqualizer_setEffectiveGain(audioequalizer *eq, float dbGain)
{
	pthread_mutex_lock(eq->eqmutex);
	eq->effgain = dbGain;
	pthread_mutex_unlock(eq->eqmutex);
}

void AudioEqualizer_setGain(audioequalizer *eq, int eqband, float dbGain)
{
	pthread_mutex_lock(eq->eqmutex);
	eq->dbGains[eqband] = dbGain;
	int i=eqband, j;
	for(j=0;j<eq->channels;j++)
		BiQuad_init(&(eq->bq[i*eq->channels+j]), eq->filtertypes[i], eq->dbGains[i], eq->freqs[i], eq->rate, eq->octave);
	if (eq->autoleveling)
	{
		AudioEqualizer_calcEffectiveGain(eq);
	}
	pthread_mutex_unlock(eq->eqmutex);
}

void AudioEqualizer_setVolume(audioequalizer *eq, float vol)
{
	pthread_mutex_lock(eq->eqmutex);
	eq->volume = vol;
	pthread_mutex_unlock(eq->eqmutex);
}

void AudioEqualizer_setEnabled(audioequalizer *eq, int enabled)
{
	pthread_mutex_lock(eq->eqmutex);
	eq->enabled = enabled;
	pthread_mutex_unlock(eq->eqmutex);
}

void AudioEqualizer_setAutoLeveling(audioequalizer *eq, int autoleveling)
{
	pthread_mutex_lock(eq->eqmutex);
	eq->autoleveling = autoleveling;
	pthread_mutex_unlock(eq->eqmutex);
}

void AudioEqualizer_close(audioequalizer *eq)
{
	int i, j;
	for(i=0;i<eq->bands;i++)
	{
		for(j=0;j<eq->channels;j++)
			BiQuad_close(&(eq->bq[i*eq->channels+j]));
		free(eq->labels[i]);
	}
	free(eq->filtertypes);
	free(eq->labels);
	free(eq->freqs);
	free(eq->bq);
	pthread_mutex_destroy(eq->eqmutex);
	free(eq->eqmutex);
}

void AudioEqualizer_BiQuadProcess(audioequalizer *eq, uint8_t *buf, int bufsize)
{
	int a, b, i, j;
	signed short *intp;

	pthread_mutex_lock(eq->eqmutex);
	if (eq->enabled)
	{
		intp=(signed short *)buf;
		float preampfactor = eq->effgain;
		float volumefactor = eq->volume;
		int samples = bufsize / snd_pcm_format_width(eq->format) * 8;
		int frames = samples / eq->channels;
		for (a=0,b=0;a<frames;a++,b+=eq->channels)
		{
			if (preampfactor < 1.0)
			{
				for(j=0;j<eq->channels;j++)
					intp[b+j] *= preampfactor;
			}

			for(i=0;i<eq->bands;i++)
			{
				for(j=0;j<eq->channels;j++)
				{
					intp[b+j] = BiQuad(&(eq->bq[i*eq->channels+j]), intp[b+j]);
				}
			}

			if (preampfactor > 1.0)
			{
				for(j=0;j<eq->channels;j++)
					intp[b+j] *= preampfactor;
			}

			for(j=0;j<eq->channels;j++) // volume
				intp[b+j] *= volumefactor;
		}
	}
	pthread_mutex_unlock(eq->eqmutex);
}

void set_eqdefaults(eqdefaults *d)
{
	float default_eqfreqs[10] = {31.0, 62.0, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0};
	char* default_eqlabels[10] = {"31", "62", "125", "250", "500", "1K", "2K", "4K", "8K", "16K"};
	filtertype default_filtertypes[10] = {LSH, PEQ, PEQ, PEQ, PEQ, PEQ, PEQ, PEQ, PEQ, HSH};
	float default_dbGains[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	//const float default_dbGains[10] = {9.0, 6.0, 3.0, 1.0, -9.0, -3.0, 9.0, 12.0, 12.0, 12.0};

	int i;
	for(i=0;i<10;i++)
	{
		d->eqfreqs[i] = default_eqfreqs[i];
		strcpy(d->eqlabels[i], default_eqlabels[i]);
		d->filtertypes[i] = default_filtertypes[i];
		d->dbGains[i] = default_dbGains[i];
	}
}

void saveto_eqdefaults(eqdefaults *d, audioequalizer *eq)
{
	int i;
	for(i=0;i<eq->bands;i++)
	{
		d->eqfreqs[i] = eq->freqs[i];
		strcpy(d->eqlabels[i], eq->labels[i]);
		d->filtertypes[i] = eq->filtertypes[i];
		d->dbGains[i] = eq->dbGains[i];
	}
}
