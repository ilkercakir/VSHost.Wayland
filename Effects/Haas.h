#ifndef Haas_h
#define Haas_h

#define _GNU_SOURCE

#include "../VSEffectShared.h"

#include "HaasS.h"

void aef_init(audioeffect *ae);
void aef_setparameter(audioeffect *ae, int i, float value);
float aef_getparameter(audioeffect *ae, int i);
void aef_process(audioeffect *ae, uint8_t* inbuffer, int inbuffersize);
void aef_reinit(audioeffect *ae);
void aef_close(audioeffect *ae);
#endif
