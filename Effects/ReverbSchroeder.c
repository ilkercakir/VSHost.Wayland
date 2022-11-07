/*
 * ReverbSchroeder.c
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

#include "ReverbSchroeder.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */
	soundreverb *r = ae->data = malloc(sizeof(soundreverb));
	set_reverbeq(&(r->reverbeqdef));

	strcpy(ae->name, "Reverb Schroeder");
	audioeffect_allocateparameters(ae, 6);
	audioeffect_initparameter(ae, 0, "Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae, 1, "Reflect", 2.0, 541.0, 79.0, 0.0, 1, pt_combo);
	audioeffect_initparameter(ae, 2, "Delays", 1.0, 24.0, 12.0, 0.0, 1, pt_combo);
	audioeffect_initparameter(ae, 3, "Feedback", 0.1, 0.9, 0.7, 0.1, 1, pt_scale);
	audioeffect_initparameter(ae, 4, "Presence", 0.1, 0.9, 0.1, 0.1, 1, pt_scale);
	audioeffect_initparameter(ae, 5, r->reverbeqdef.eqlabels[0], 500.0, 10000.0, r->reverbeqdef.eqfreqs[0], 100.0, 1, pt_scale);

	soundreverb_init((int)ae->parameter[1].value, (int)ae->parameter[2].value, ae->parameter[3].value, ae->parameter[4].value, &(r->reverbeqdef), ae->format, ae->rate, ae->channels, r);

//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	soundreverb *r = (soundreverb *)ae->data;

	switch(i)
	{
		case 0: // Enable
			break;
		case 1: // Reflect
			break;
		case 2: // Delay ms
			break;
		case 3: // Feedback
			break;
		case 4: // Presence
			break;
		case 5: // EQ Band 0
			r->reverbeqdef.eqfreqs[0] = ae->parameter[i].value;
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
 
	soundreverb *r = (soundreverb *)ae->data;

	if (ae->parameter[0].value)
		soundreverb_add((char*)inbuffer, inbuffersize, r);

/* User defined processing code end */
}

void aef_reinit(audioeffect *ae)
{
/* User defined reinitialization code begin */

	soundreverb *r = (soundreverb *)ae->data;

	soundreverb_reinit((int)ae->parameter[1].value, (int)ae->parameter[2].value, ae->parameter[3].value, ae->parameter[4].value, &(r->reverbeqdef), r);

//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined reinitialization code end */
}

void aef_close(audioeffect *ae)
{
	audioeffect_deallocateparameters(ae);

/* User defined cleanup code begin  */

	soundreverb *r = (soundreverb *)ae->data;

	soundreverb_close(r);

	free(ae->data);

/* User defined cleanup code end */
}

// Delay Processor in DelayS.c

// Schroeder's Reverb

void set_reverbeq(eqdefaults *d)
{
	float default_eqfreqs[1] = {4000.0};
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

int soundreverb_initprimes_callback(void *data, int argc, char **argv, char **azColName) 
{
	soundreverb *r = (soundreverb *)data;

//    for (int i = 0; i < argc; i++)
//    {
//     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
//    }
	r->reverbprimes[r->i++] = atof(argv[1]);

	return 0;
}

void soundreverb_initprimes(soundreverb *r)
{
	sqlite3 *db;
	char *err_msg = NULL;
	char sql[200];
	int rc;
	int i;

	r->i = 0;

	if ((rc = sqlite3_open("/var/sqlite3DATA/mediaplayer.db", &db)))
	{
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
	}
	else
	{
//printf("Opened database successfully\n");
		sprintf(sql, "select * from primes where prime >= %d order by id limit %d;", r->reflect, r->reverbdelaylines);
		if ((rc = sqlite3_exec(db, sql, soundreverb_initprimes_callback, (void*)r, &err_msg)) != SQLITE_OK)
		{
			printf("Failed to select data, %s\n", err_msg);
			sqlite3_free(err_msg);
		}
		else
		{
// success
		}
	}
	sqlite3_close(db);

//printf("%d initialized\n", r->i);
//	for(i=0;i<r->i;i++)
//		printf("%5.2f\n", r->reverbprimes[i]);

	r->alpha = -log(r->feedback) / (r->reverbprimes[0]/1000.0);
	for(i=0;i<r->reverbdelaylines;i++)
	{
		r->feedbacks[i] = exp(-r->alpha*r->reverbprimes[i]/1000.0);
		//printf("%d\t%f\n", i, r->feedbacks[i]);
	}
}

void soundreverb_reinit(int reflect, int delaylines, float feedback, float presence, eqdefaults *d, soundreverb *r)
{
	int i;

	AudioEqualizer_close(&(r->reverbeq));

	for(i=0;i<r->reverbdelaylines;i++)
	{
		sounddelay_close(&(r->snddlyrev[i]));
	}

	if (r->bbuf)
	{
		free(r->bbuf);
		r->bbuf = NULL;
	}
	free(r->reverbprimes);
	r->reverbprimes = NULL;
	free(r->feedbacks);
	r->feedbacks = NULL;
	free(r->snddlyrev);	
	r->snddlyrev = NULL;

	r->reflect = reflect;
	r->reverbdelaylines = delaylines;
	r->feedback = feedback;
	r->presence = presence;

	r->reverbprimes = malloc(r->reverbdelaylines*sizeof(float));
	r->feedbacks = malloc(r->reverbdelaylines*sizeof(float));
	r->snddlyrev = malloc(r->reverbdelaylines*sizeof(sounddelay));

	soundreverb_initprimes(r);
	for(i=0;i<r->reverbdelaylines;i++)
	{
		sounddelay_init(r->reverbdelaylines, DLY_REVERB, r->reverbprimes[i], r->feedbacks[i], r->format, r->rate, r->channels, &(r->snddlyrev[i]));
	}

	r->bbuf = NULL;
	r->eqoctave = 1.0;

	AudioEqualizer_init(&(r->reverbeq), 1, r->eqoctave, 1, 1, r->format, r->rate, r->channels, d);
}

void soundreverb_init(int reflect, int delaylines, float feedback, float presence, eqdefaults *d, snd_pcm_format_t format, unsigned int rate, unsigned int channels, soundreverb *r)
{
	int i;

	r->format = format;
	r->rate = rate;
	r->channels = channels;
	r->reflect = reflect;
	r->reverbdelaylines = delaylines;
	r->feedback = feedback;
	r->presence = presence;

	r->reverbprimes = malloc(r->reverbdelaylines*sizeof(float));
	r->feedbacks = malloc(r->reverbdelaylines*sizeof(float));
	r->snddlyrev = malloc(r->reverbdelaylines*sizeof(sounddelay));

	soundreverb_initprimes(r);
	for(i=0;i<r->reverbdelaylines;i++)
	{
		sounddelay_init(r->reverbdelaylines, DLY_REVERB, r->reverbprimes[i], r->feedbacks[i], format, rate, channels, &(r->snddlyrev[i]));
	}

	r->bbuf = NULL;
	r->eqoctave = 1.0;

	AudioEqualizer_init(&(r->reverbeq), 1, r->eqoctave, 1, 1, r->format, r->rate, r->channels, d);
}

void soundreverb_add(char* inbuffer, int inbuffersize, soundreverb *r)
{
	int i, j, *readfront, *fbsamples;
	signed short *dstbuf, *srcbuf;

	for(i=0;i<r->reverbdelaylines;i++)
	{	
		sounddelay_add(inbuffer, inbuffersize, &(r->snddlyrev[i]));
	}

	if (!r->bbuf)
	{
		r->bbuf = malloc(inbuffersize);
	}

	memset(r->bbuf, 0, inbuffersize);
	dstbuf = (signed short*)r->bbuf;
	for(i=0;i<r->reverbdelaylines;i++)
	{
		srcbuf = (signed short*)r->snddlyrev[i].fbuffer;
		readfront = &(r->snddlyrev[i].readfront);
		fbsamples = &(r->snddlyrev[i].fbuffersamples);
		for(j=0;j<r->snddlyrev[i].insamples;j++)
		{
			dstbuf[j] += srcbuf[(*readfront)++] * r->presence;
			(*readfront)%=(*fbsamples);
		}
	}

	AudioEqualizer_BiQuadProcess(&(r->reverbeq), (uint8_t*)r->bbuf, inbuffersize);

	dstbuf = (signed short*)inbuffer;
	srcbuf = (signed short*)r->bbuf;
	for(j=0; j<r->snddlyrev[0].insamples; j++)
	{
		dstbuf[j] += srcbuf[j];
	}
}

void soundreverb_close(soundreverb *r)
{
	int i;

	AudioEqualizer_close(&(r->reverbeq));
	for(i=0;i<r->reverbdelaylines;i++)
	{
		sounddelay_close(&(r->snddlyrev[i]));
	}
		
	if (r->bbuf)
	{
		free(r->bbuf);
		r->bbuf = NULL;
	}
	free(r->reverbprimes);
	r->reverbprimes = NULL;
	free(r->feedbacks);
	r->feedbacks = NULL;
	free(r->snddlyrev);	
	r->snddlyrev = NULL;
}
