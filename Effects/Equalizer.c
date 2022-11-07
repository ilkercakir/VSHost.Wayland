/*
 * Equalizer.c
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

#include "Equalizer.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */

	equalizer *equalizerdata = ae->data = malloc(sizeof(equalizer));
	audioequalizer *eq = &(equalizerdata->eq);
	eqdefaults *eqd = &(equalizerdata->eqdef);

	set_eqdefaults(eqd);

	strcpy(ae->name, "Equalizer");
	audioeffect_allocateparameters(ae, 14);
	audioeffect_initparameter(ae,  0, "Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae,  1, eqd->eqlabels[0], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae,  2, eqd->eqlabels[1], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae,  3, eqd->eqlabels[2], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae,  4, eqd->eqlabels[3], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae,  5, eqd->eqlabels[4], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae,  6, eqd->eqlabels[5], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae,  7, eqd->eqlabels[6], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae,  8, eqd->eqlabels[7], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae,  9, eqd->eqlabels[8], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae, 10, eqd->eqlabels[9], -12.0, 12.0, 0.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae, 11, "Gain", 0.0, 2.0, 1.0, 0.1, 0, pt_scale);
	audioeffect_initparameter(ae, 12, "Auto", 0.0, 1.0, 1.0, 1.0, 0, pt_switch);
	audioeffect_initparameter(ae, 13, "Preset", 0.0, 6.0, 0.0, 1.0, 0, pt_combo);

	equalizerdata->format = ae->format;
	equalizerdata->rate = ae->rate;
	equalizerdata->channels = ae->channels;

	AudioEqualizer_init(eq, 10, 1.0, (int)aef_getparameter(ae, 0), (int)aef_getparameter(ae, 12), equalizerdata->format, equalizerdata->rate, equalizerdata->channels, eqd);
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	equalizer *equalizerdata = (equalizer *)ae->data;
	audioequalizer *eq = &(equalizerdata->eq);

	switch(i)
	{
		case 0: // Enable
			eq->enabled = (int)ae->parameter[i].value;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			AudioEqualizer_setGain(eq, i-1, ae->parameter[i].value);
			if (eq->autoleveling)
				audioeffect_setdependentparameter(ae, 11, eq->effgain);
			break;
		case 11:
			eq->effgain = ae->parameter[i].value;
			break;
		case 12:
			eq->autoleveling = ae->parameter[i].value;
			break;
		case 13:
			switch ((int)ae->parameter[i].value)
			{
				case 0:
					audioeffect_setdependentparameter(ae, 1, 0.0);
					audioeffect_setdependentparameter(ae, 2, 0.0);
					audioeffect_setdependentparameter(ae, 3, 0.0);
					audioeffect_setdependentparameter(ae, 4, 0.0);
					audioeffect_setdependentparameter(ae, 5, 0.0);
					audioeffect_setdependentparameter(ae, 6, 0.0);
					audioeffect_setdependentparameter(ae, 7, 0.0);
					audioeffect_setdependentparameter(ae, 8, 0.0);
					audioeffect_setdependentparameter(ae, 9, 0.0);
					audioeffect_setdependentparameter(ae, 10, 0.0);
					break;
				case 1:
					audioeffect_setdependentparameter(ae, 1, 4.6);
					audioeffect_setdependentparameter(ae, 2, 4.6);
					audioeffect_setdependentparameter(ae, 3, 2.3);
					audioeffect_setdependentparameter(ae, 4, 0.0);
					audioeffect_setdependentparameter(ae, 5, -5.4);
					audioeffect_setdependentparameter(ae, 6, -5.4);
					audioeffect_setdependentparameter(ae, 7, 0.0);
					audioeffect_setdependentparameter(ae, 8, 3.0);
					audioeffect_setdependentparameter(ae, 9, 5.9);
					audioeffect_setdependentparameter(ae, 10, 5.9);
					break;				
				case 2:
					audioeffect_setdependentparameter(ae, 1, 2.4);
					audioeffect_setdependentparameter(ae, 2, 1.8);
					audioeffect_setdependentparameter(ae, 3, 0.0);
					audioeffect_setdependentparameter(ae, 4, -2.4);
					audioeffect_setdependentparameter(ae, 5, -2.4);
					audioeffect_setdependentparameter(ae, 6, -1.6);
					audioeffect_setdependentparameter(ae, 7, 2.7);
					audioeffect_setdependentparameter(ae, 8, 3.7);
					audioeffect_setdependentparameter(ae, 9, 4.0);
					audioeffect_setdependentparameter(ae, 10, 4.0);
					break;
				case 3:
					audioeffect_setdependentparameter(ae, 1, 1.6);
					audioeffect_setdependentparameter(ae, 2, 3.7);
					audioeffect_setdependentparameter(ae, 3, 1.8);
					audioeffect_setdependentparameter(ae, 4, -1.1);
					audioeffect_setdependentparameter(ae, 5, -0.8);
					audioeffect_setdependentparameter(ae, 6, 0.5);
					audioeffect_setdependentparameter(ae, 7, 1.6);
					audioeffect_setdependentparameter(ae, 8, 3.2);
					audioeffect_setdependentparameter(ae, 9, 4.3);
					audioeffect_setdependentparameter(ae, 10, 4.6);
					break;
				case 4:
					audioeffect_setdependentparameter(ae, 1, -0.5);
					audioeffect_setdependentparameter(ae, 2, 1.6);
					audioeffect_setdependentparameter(ae, 3, 2.4);
					audioeffect_setdependentparameter(ae, 4, 2.6);
					audioeffect_setdependentparameter(ae, 5, 1.8);
					audioeffect_setdependentparameter(ae, 6, 0.0);
					audioeffect_setdependentparameter(ae, 7, -0.8);
					audioeffect_setdependentparameter(ae, 8, -0.8);
					audioeffect_setdependentparameter(ae, 9, -0.5);
					audioeffect_setdependentparameter(ae, 10, -0.5);
					break;
				case 5:
					audioeffect_setdependentparameter(ae, 1, 2.6);
					audioeffect_setdependentparameter(ae, 2, 1.6);
					audioeffect_setdependentparameter(ae, 3, -1.8);
					audioeffect_setdependentparameter(ae, 4, -2.6);
					audioeffect_setdependentparameter(ae, 5, -1.1);
					audioeffect_setdependentparameter(ae, 6, 1.3);
					audioeffect_setdependentparameter(ae, 7, 2.9);
					audioeffect_setdependentparameter(ae, 8, 3.7);
					audioeffect_setdependentparameter(ae, 9, 3.7);
					audioeffect_setdependentparameter(ae, 10, 3.7);
					break;
				case 6:
					audioeffect_setdependentparameter(ae, 1, 2.6);
					audioeffect_setdependentparameter(ae, 2, 1.8);
					audioeffect_setdependentparameter(ae, 3, 0.0);
					audioeffect_setdependentparameter(ae, 4, -2.6);
					audioeffect_setdependentparameter(ae, 5, -1.6);
					audioeffect_setdependentparameter(ae, 6, 0.0);
					audioeffect_setdependentparameter(ae, 7, 2.6);
					audioeffect_setdependentparameter(ae, 8, 3.2);
					audioeffect_setdependentparameter(ae, 9, 3.2);
					audioeffect_setdependentparameter(ae, 10, 2.6);
					break;
			}
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
 
	equalizer *equalizerdata = (equalizer *)ae->data;
	audioequalizer *eq = &(equalizerdata->eq);

	AudioEqualizer_BiQuadProcess(eq, inbuffer, inbuffersize);

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

	equalizer *equalizerdata = (equalizer *)ae->data;
	audioequalizer *eq = &(equalizerdata->eq);

	AudioEqualizer_close(eq);

	free(ae->data);

/* User defined cleanup code end */
}

// Eq Processor in BiQuad.c
