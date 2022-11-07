/*
 * Effect1.c
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

#include "Effect1.h"

void aef_init(audioeffect *ae)
{
/* User defined initialization code begin */

	strcpy(ae->name, "Effect1");
	audioeffect_allocateparameters(ae, 6);
	audioeffect_initparameter(ae, 0, "parm1", 0.0, 1.0, 0.0, 1.0, 1, pt_switch);
	audioeffect_initparameter(ae, 1, "parm2", -5.0, 10.0, 5.0, 1.0, 0, pt_scale);
	audioeffect_initparameter(ae, 2, "parm3", -5.0, 10.0, 5.0, 1.0, 0, pt_check);
	audioeffect_initparameter(ae, 3, "parm4", -5.0, 10.0, 5.0, 0.1, 0, pt_spin);
	audioeffect_initparameter(ae, 4, "parm5", 0.0, 1.0, 0.0, 1.0, 0, pt_combo);
	audioeffect_initparameter(ae, 5, "parm6", -5.0, 10.0, 5.0, 1.0, 0, pt_scale);

// malloc ae->data

/* User defined initialization code end */
}

void aef_setparameter(audioeffect *ae, int i, float value)
{
	ae->parameter[i].value = value;

/* User defined parameter setter code begin */

	switch(i)
	{
		case 0: // when resetrequired defer "ae->data.parametervalue = ae->parameter[i].value" to aef_reinit()
			break;
		case 1: // when dependent parameters exist
			audioeffect_setdependentparameter(ae, 5, ae->parameter[i].value); // set dependent parameter value and visualization
			break;
		case 2: // when not resetrequired
			// ae->data.parametervalue = ae->parameter[i].value;
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



/* User defined processing code end */
}

void aef_reinit(audioeffect *ae)
{
/* User defined reinitialization code begin */

// free ae->data.parameter
// ae->data.parametervalue = ae->parameter[i].value
// malloc ae->data.parameter

/* User defined reinitialization code end */
}

void aef_close(audioeffect *ae)
{
	audioeffect_deallocateparameters(ae);

/* User defined cleanup code begin */
 
// free ae->data

/* User defined cleanup code end */
}
