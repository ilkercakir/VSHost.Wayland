#ifndef VSUtil_h
#define VSUtil_h

#define _GNU_SOURCE

#include <stdint.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <sqlite3.h>

#include "VStudio.h"
#include "VSUtil.h"

void virtualstudio_set_dbpath(virtualstudio *vs, char *dbpath);
void virtualstudio_purge(virtualstudio *vs);
void virtualstudio_addeffect(virtualstudio *vs, char *path);
void virtualstudio_deleteeffect(virtualstudio *vs, int id);
void virtualstudio_addchain(virtualstudio *vs, char *name);
void virtualstudio_deletechain(virtualstudio *vs, int chain);
void virtualstudio_assign_effect(virtualstudio *vs, int chain, int effect);
void virtualstudio_unassign_effect(virtualstudio *vs, int chain, int effect);
#endif
