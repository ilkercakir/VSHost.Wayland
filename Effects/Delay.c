/*
 * Delay.c
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

#include "Delay.h"

void aef_init(audioeffect *ae)
{
/* User defined parameter initialization code begin */

	strcpy(ae->name, "Delay");
	audioeffect_allocateparameters(ae, 4);
	audioeffect_initparameter(ae, 0, "Enable", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae, 1, "Type", 0.0, 1.0, 0.0, 1.0, 0, pt_combo);
	audioeffect_initparameter(ae, 2, "Delay ms", 1.0, 2000.0, 500.0, 1.0, 1, pt_scale);
	audioeffect_initparameter(ae, 3, "Feedback", 0.1, 0.9, 0.6, 0.1, 0, pt_scale);

	sounddelay *d = ae->data = malloc(sizeof(sounddelay));

	sounddelay_init(1, (dly_type)ae->parameter[1].value, ae->parameter[2].value, ae->parameter[3].value, ae->format, ae->rate, ae->channels, d);
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined parameter initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	sounddelay *d = (sounddelay *)ae->data;

	switch(i)
	{
		case 0: // Enable
			break;
		case 1: // Type
			d->delaytype = (dly_type)ae->parameter[i].value;
			break;
		case 2: // Delay ms
			break;
		case 3: // Feedback
			d->feedback = ae->parameter[i].value;
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
 
	sounddelay *d = (sounddelay *)ae->data;

	if (ae->parameter[0].value)
		sounddelay_add((char*)inbuffer, inbuffersize, d);

/* User defined processing code end */
}

void aef_reinit(audioeffect *ae)
{
/* User defined reinitialization code begin */

	sounddelay *d = (sounddelay *)ae->data;

	sounddelay_reinit(1, (dly_type)ae->parameter[1].value, ae->parameter[2].value, ae->parameter[3].value, d);
//printf("%f, %f, %f, %f\n", aef_getparameter(ae, 0), aef_getparameter(ae, 1), aef_getparameter(ae, 2), aef_getparameter(ae, 3));

/* User defined reinitialization code end */
}

void aef_close(audioeffect *ae)
{
	audioeffect_deallocateparameters(ae);

/* User defined cleanup code begin  */

	sounddelay *d = (sounddelay *)ae->data;

	sounddelay_close(d);

	free(ae->data);

/* User defined cleanup code end */
}

// Delay Processor in DelayS.c
