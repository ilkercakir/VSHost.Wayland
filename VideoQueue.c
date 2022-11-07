/*
 * VideoQueue.c
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

#include "VideoQueue.h"

void vq_init(videoplayerqueue *vpq, int maxLength)
{
	int ret;

	vpq->vq = NULL;
	vpq->vqLength = 0;
	vpq->vqMaxLength = maxLength;

	if((ret=pthread_mutex_init(&(vpq->vqmutex), NULL))!=0 )
		printf("vqmutex init failed, %d\n", ret);

	if((ret=pthread_cond_init(&(vpq->vqlowcond), NULL))!=0 )
		printf("vqlowcond init failed, %d\n", ret);

	if((ret=pthread_cond_init(&(vpq->vqhighcond), NULL))!=0 )
		printf("vqhighcond init failed, %d\n", ret);

	if((ret=pthread_cond_init(&(vpq->vqxcond), NULL))!=0 )
		printf("vqxcond init failed, %d\n", ret);

	vpq->playerstatus = PLAYING;
}

void vq_add(videoplayerqueue *vpq,  char *yuv, int64_t label)
{
	videoqueue *p;

	pthread_mutex_lock(&(vpq->vqmutex));
	while (vpq->vqLength>=vpq->vqMaxLength)
	{
		//printf("Video queue sleeping, overrun\n");
		pthread_cond_wait(&(vpq->vqhighcond), &(vpq->vqmutex));
	}

	p = malloc(sizeof(videoqueue));
//printf("malloc vq %d\n", sizeof(struct videoqueue));
	if (vpq->vq == NULL)
	{
		p->next = p;
		p->prev = p;
		vpq->vq = p;
	}
	else
	{
		p->next = vpq->vq;
		p->prev = vpq->vq->prev;
		vpq->vq->prev = p;
		p->prev->next = p;
	}
	p->yuv = yuv;
	p->label = label;

	vpq->vqLength++;

	//condition = true;
	pthread_cond_signal(&(vpq->vqlowcond)); // Should wake up *one* thread
	pthread_mutex_unlock(&(vpq->vqmutex));
}

videoqueue* vq_remove_element(videoqueue **q)
{
	videoqueue *p;

	if ((*q)->next == (*q))
	{
		p=*q;
		*q = NULL;
	}
	else
	{
		p = (*q);
		(*q) = (*q)->next;
		(*q)->prev = p->prev;
		(*q)->prev->next = (*q);
	}
	return p;
}

videoqueue* vq_remove(videoplayerqueue *vpq)
{
	videoqueue *p;

	pthread_mutex_lock(&(vpq->vqmutex));
	while(vpq->vq==NULL) // queue empty
	{
		if ((vpq->playerstatus==PLAYING) || (vpq->playerstatus==PAUSED))
		{
			//printf("Video queue sleeping, underrun\n");
			pthread_cond_wait(&(vpq->vqlowcond), &(vpq->vqmutex));
		}
		else
			break;
	}
	switch (vpq->playerstatus)
	{
		case PLAYING:
		case PAUSED:
			p = vq_remove_element(&(vpq->vq));
			vpq->vqLength--;
			break;
		case DRAINING:
			if (vpq->vqLength>0)
			{
				p = vq_remove_element(&(vpq->vq));
				vpq->vqLength--;
			}
			else
				p=NULL;
			break;
		default:
			p = NULL;
			break;
	}

	//condition = true;
	pthread_cond_signal(&(vpq->vqhighcond)); // Should wake up *one* thread
	pthread_mutex_unlock(&(vpq->vqmutex));

	return p;
}

void vq_requeststop(videoplayerqueue *vpq)
{
	pthread_mutex_lock(&(vpq->vqmutex));
	if (vpq->playerstatus != IDLE)
	{
		vpq->playerstatus = DRAINING;
		pthread_cond_signal(&(vpq->vqlowcond)); // Should wake up *one* thread
	}
	pthread_mutex_unlock(&(vpq->vqmutex));
}

void vq_signalstop(videoplayerqueue *vpq)
{
	pthread_mutex_lock(&(vpq->vqmutex));
	vpq->playerstatus = IDLE;
	pthread_cond_signal(&(vpq->vqxcond)); // Should wake up *one* thread
	pthread_mutex_unlock(&(vpq->vqmutex));
}

void vq_drain(videoplayerqueue *vpq)
{
	pthread_mutex_lock(&(vpq->vqmutex));
	while (vpq->playerstatus!=IDLE)
		pthread_cond_wait(&(vpq->vqxcond), &(vpq->vqmutex));
	pthread_mutex_unlock(&(vpq->vqmutex));
}

void vq_destroy(videoplayerqueue *vpq)
{
	pthread_mutex_destroy(&(vpq->vqmutex));
	pthread_cond_destroy(&(vpq->vqlowcond));
	pthread_cond_destroy(&(vpq->vqhighcond));
	pthread_cond_destroy(&(vpq->vqxcond));
}
