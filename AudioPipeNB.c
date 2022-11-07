/*
 * AudioPipe.c
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

#include "AudioPipeNB.h"

void audioCQ_init(audiopipe *p, snd_pcm_format_t format, unsigned int rate, unsigned int channels, unsigned int frames, unsigned int cqbufferframes)
{
	int ret;

	p->format = format;
	p->rate = rate;
	p->channels = channels;

	p->front = p->rear = 0;

	p->frames = frames;
	p->buffersize = frames * p->channels * snd_pcm_format_width(p->format) / 8; // out buffer size is quantized
	p->buffer = malloc(p->buffersize);

	p->cqbufferframes = cqbufferframes;
	p->cqbuffersize = p->cqbufferframes * p->channels * snd_pcm_format_width(p->format) / 8;
	p->cqbuffer = malloc(p->cqbuffersize);

	if((ret=pthread_mutex_init(&(p->pipemutex), NULL))!=0 )
		printf("pipe mutex init failed, %d\n", ret);

	if((ret=pthread_cond_init(&(p->pipehighcond), NULL))!=0 )
		printf("pipe high cond init failed, %d\n", ret);

	p->status = CQ_RUNNING;
}

void audioCQ_add(audiopipe *p, char *inbuffer, int inbuffersize)
{
	int offset, bytesleft, bytestocopy;

	pthread_mutex_lock(&(p->pipemutex));
	while ((bytesleft = p->cqbuffersize - ((p->rear + p->cqbuffersize - p->front) % p->cqbuffersize) - 1) < inbuffersize)
	{
		if (p->status == CQ_STOPPED)
		{
			pthread_mutex_unlock(&(p->pipemutex));
			return;
		}
		pthread_cond_wait(&(p->pipehighcond), &(p->pipemutex));
	}
	for(offset=0,bytesleft=inbuffersize;bytesleft;bytesleft-=bytestocopy)
	{
		bytestocopy = (p->rear+bytesleft>p->cqbuffersize?p->cqbuffersize-p->rear:bytesleft);
		memcpy(p->cqbuffer+p->rear, inbuffer+offset, bytestocopy);
		offset += bytestocopy;
		p->rear += bytestocopy; p->rear %= p->cqbuffersize;
	}
	p->inbufferframes = inbuffersize / ( p->channels * snd_pcm_format_width(p->format) / 8 );
//printf("cq_add %d\n", inbuffersize);
	pthread_mutex_unlock(&(p->pipemutex));
}

audioCQstatus audioCQ_remove(audiopipe *p)
{
	int offset, bytesleft, bytestocopy, length, ret;

	pthread_mutex_lock(&(p->pipemutex));
	length = (p->rear - p->front + p->cqbuffersize) % p->cqbuffersize;
	if (length<p->buffersize)
	{
		memset(p->buffer, 0, p->buffersize);
		ret = p->status;
	}
	else
	{
		for(offset=0,bytesleft=p->buffersize;bytesleft;bytesleft-=bytestocopy)
		{
			bytestocopy = (p->front+bytesleft>p->cqbuffersize?p->cqbuffersize-p->front:bytesleft);
			memcpy(p->buffer+offset, p->cqbuffer+p->front, bytestocopy);
			offset += bytestocopy;
			p->front += bytestocopy; p->front %= p->cqbuffersize;
		}
		pthread_cond_signal(&(p->pipehighcond));
		ret = p->status;
	}
	pthread_mutex_unlock(&(p->pipemutex));
	return(ret);
}

audioCQstatus audioCQ_removeVB(audiopipe *p, char *outbuffer, int outbuffersize)
{
	int offset, bytesleft, bytestocopy, length, ret;

	pthread_mutex_lock(&(p->pipemutex));
	length = (p->rear - p->front + p->cqbuffersize) % p->cqbuffersize;
	if (length<outbuffersize)
	{
		memset(outbuffer, 0, outbuffersize);
		ret = p->status;
//printf("cq_remove null buffer %d\n", outbuffersize);
	}
	else
	{
		for(offset=0,bytesleft=outbuffersize;bytesleft;bytesleft-=bytestocopy)
		{
			bytestocopy = (p->front+bytesleft>p->cqbuffersize?p->cqbuffersize-p->front:bytesleft);
			memcpy(outbuffer+offset, p->cqbuffer+p->front, bytestocopy);
			offset += bytestocopy;
			p->front += bytestocopy; p->front %= p->cqbuffersize;
		}
		pthread_cond_signal(&(p->pipehighcond));
		ret = p->status;
//printf("cq_remove %d\n", outbuffersize);
	}
	pthread_mutex_unlock(&(p->pipemutex));
	return(ret);
}

void audioCQ_signalstop(audiopipe *p)
{
	pthread_mutex_lock(&(p->pipemutex));
	p->status = CQ_STOPPED;
	pthread_cond_signal(&(p->pipehighcond));
	pthread_mutex_unlock(&(p->pipemutex));
}

void audioCQ_close(audiopipe *p)
{
	pthread_cond_destroy(&(p->pipehighcond));
	pthread_mutex_destroy(&(p->pipemutex));
	free(p->cqbuffer);
	free(p->buffer);
	p->status = CQ_STOPPED;
}
