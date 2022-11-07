#ifndef VideoQueueH
#define VideoQueueH

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

typedef enum
{
	IDLE = 0,
	PLAYING,
	PAUSED,
	DRAINING
}vpstatus;

typedef struct videoqueue
{
	struct videoqueue *prev;
	struct videoqueue *next;
	char *yuv; // y,u,v byte arrays concatenated
	int64_t label;
}videoqueue;

typedef struct
{
	videoqueue *vq;
	int vqLength;
	int vqMaxLength;
	pthread_mutex_t vqmutex;
	pthread_cond_t vqlowcond;
	pthread_cond_t vqhighcond;
	pthread_cond_t vqxcond;
	vpstatus playerstatus;
}videoplayerqueue;

void vq_init(videoplayerqueue *vpq, int maxLength);
void vq_add(videoplayerqueue *vpq,  char *yuv, int64_t label);
videoqueue* vq_remove_element(videoqueue **q);
videoqueue* vq_remove(videoplayerqueue *vpq);
void vq_requeststop(videoplayerqueue *vpq);
void vq_signalstop(videoplayerqueue *vpq);
void vq_drain(videoplayerqueue *vpq);
void vq_destroy(videoplayerqueue *vpq);
#endif
