/*
 * AudioMixer.c
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

#include "AudioMixer.h"

void allocate_audiochannel(int channelbuffers, snd_pcm_format_t format, int rate, int frames, int channels, audiochannel **c)
{
	(*c) = malloc(sizeof(audiochannel));

	(*c)->channelframes = frames;
	(*c)->channelsamples = (*c)->channelframes * channels;
	(*c)->channelbuffersize = (*c)->channelsamples * ( snd_pcm_format_width(format) / 8 );
	(*c)->channelbuffers = channelbuffers;

//printf("allocating channel[%d]\n", id);
	(*c)->channelbuffer = malloc((*c)->channelbuffers * sizeof(char*));
	(*c)->cshort = malloc((*c)->channelbuffers * sizeof(signed short*));
	int i;
	for(i=0;i<(*c)->channelbuffers;i++)
	{
		(*c)->channelbuffer[i] = malloc((*c)->channelbuffersize * sizeof(char));
		(*c)->cshort[i] = (signed short *)((*c)->channelbuffer[i]);
	}

	(*c)->front = (*c)->rear = 0;
	(*c)->readoffset = 0;
//printf("allocate_audiochannel\n");
}

void deallocate_audiochannel(audiochannel **c)
{
	int i;
	for(i=0;i<(*c)->channelbuffers;i++)
		free((*c)->channelbuffer[i]);
	free((*c)->channelbuffer);
	free((*c)->cshort);
	free(*c);
	(*c) = NULL;
//printf("deallocate_audiochannel\n");
}

void init_audiomixer(int mixerchannels, xblock blocking, snd_pcm_format_t format, int rate, int frames, int channels, audiomixer *x)
{
	int ret;

	x->xmutex = malloc(sizeof(pthread_mutex_t));
	x->xlowcond = malloc(sizeof(pthread_cond_t));
	x->xhighcond = malloc(sizeof(pthread_cond_t));
	x->xchannelcond = malloc(sizeof(pthread_cond_t));

	if((ret=pthread_mutex_init(x->xmutex, NULL))!=0 )
		printf("mutex init failed, %d\n", ret);

	if((ret=pthread_cond_init(x->xlowcond, NULL))!=0 )
		printf("cond init failed, %d\n", ret);

	if((ret=pthread_cond_init(x->xhighcond, NULL))!=0 )
		printf("cond init failed, %d\n", ret);

	if((ret=pthread_cond_init(x->xchannelcond, NULL))!=0 )
		printf("cond init failed, %d\n", ret);

	x->format = format;
	x->rate = rate;
	x->channels = channels;
	x->outbufferframes = frames;

	x->front = x->rear = 0;
	x->outbuffersamples = x->outbufferframes * x->channels;
	x->outbuffersize = x->outbuffersamples * ( snd_pcm_format_width(x->format) / 8 );

	//printf("allocating buffer %d\n", q->aqLength * sizeof(char*));
	x->outbuffer = malloc(x->outbuffersize);
	x->outshort = (signed short *)x->outbuffer;

	x->blocking = blocking;
	x->channelcount = mixerchannels;
	x->ac = malloc(x->channelcount*sizeof(audiochannel *));
	int i;
	for(i=0;i<x->channelcount;i++)
		x->ac[i] = NULL;

	x->prescale = 1.0 / sqrt(x->channelcount);
	x->mixerstatus = MX_RUNNING;
//printf("mixer init %d channels\n", x->channelcount);
}

int allocatechannel_audiomixer(int frames, int channelbuffers, audiomixer *x)
{
	int i=0, channel=-1;
	for(i=0;i<x->channelcount;i++)
	{
		if (!(x->ac[i]))
		{
			channel = i;
//printf("allocatechannel_audiomixer %d\n", channel);
			allocate_audiochannel(channelbuffers, x->format, x->rate, frames, x->channels, &(x->ac[channel]));
			break;
		}
	}
	for(i=0, x->activechannels=0;i<x->channelcount;i++)
		if (x->ac[i]) x->activechannels++;
	if (x->activechannels)
		x->prescale = 1.0 / sqrt(x->activechannels);
	else
		x->prescale = 1.0 / sqrt(x->channelcount);

	return channel;
}

void deallocatechannel_audiomixer(int channel, audiomixer *x)
{
	int i;
//printf("deallocate_audiochannel\n");
	deallocate_audiochannel(&(x->ac[channel]));
	pthread_cond_signal(x->xlowcond); // Should wake up *one* thread

	for(i=0, x->activechannels=0;i<x->channelcount;i++)
		if (x->ac[i]) x->activechannels++;
	if (x->activechannels)
		x->prescale = 1.0 / sqrt(x->activechannels);
	else
		x->prescale = 1.0 / sqrt(x->channelcount);

	pthread_cond_signal(x->xchannelcond); // Should wake up *one* thread
}

mxstatus readfrommixer(audiomixer *x)
{
	pthread_mutex_lock(x->xmutex);
	memset(x->outbuffer, 0, x->outbuffersize);
	int i, j;
	for(i=0;i<x->channelcount;i++)
	{
		if (x->ac[i])
		{
//printf("reading %d, offset %d\n", i, x->ac[i]->readoffset);
			char **channelbuffer = x->ac[i]->channelbuffer;
			int *rear = &(x->ac[i]->rear);
			int *front = &(x->ac[i]->front);
			int *readoffset = &(x->ac[i]->readoffset);
			int writeoffset = 0;
			int samplestoread = x->outbuffersamples;
			while (samplestoread)
			{
				signed short *fshort = (signed short *)(channelbuffer[(*front)]);
				int samplesreadable = x->ac[i]->channelsamples - (*readoffset);
				samplesreadable = (samplesreadable>samplestoread?samplestoread:samplesreadable);
//if (i==1) printf("reading %d samples from channel %d\n", samplesreadable, i);
				if (x->blocking)
				{
					while ((*front) == (*rear)) // queue empty
					{
						if (x->mixerstatus == MX_STOPPED) // mixer was stopped
						{
							pthread_mutex_unlock(x->xmutex);
							return(x->mixerstatus);
						}
//printf("sleeping, audio queue empty, channel %d\n", i);
						pthread_cond_wait(x->xlowcond, x->xmutex);
						if (!x->ac[i]) // channel was closed
						{
							samplestoread = samplesreadable = 0;
							break;
						}
					}
//if (i==1) printf("channel %d reading %d samples from %d %d to %d\n", i, samplesreadable, (*front), (*readoffset), writeoffset);
					for(j=0;j<samplesreadable;j++,samplestoread--)
					{
						x->outshort[writeoffset++] += fshort[(*readoffset)++] * x->prescale;
					}
//if (i==1) printf("channel %d done\n", i);
					if (x->ac[i]) // channel not closed
					{
						(*readoffset)%=x->ac[i]->channelsamples;
						if (!(*readoffset))
						{
							(*front)++;
							(*front)%=x->ac[i]->channelbuffers;
							//pthread_cond_signal(x->xhighcond); // Should wake up *one* thread
							pthread_cond_broadcast(x->xhighcond); // Should wake up *all* threads
						}
					}
				}
				else
				{
					if ((*front) == (*rear))
					{
						samplestoread = 0;
//printf("channel %d skipping %d samples from %d to %d\n", i, samplesreadable, (*readoffset), writeoffset);
					}
					else
					{
//printf("channel %d writing %d samples from %d to %d\n", i, samplesreadable, (*readoffset), writeoffset);
						for(j=0;j<samplesreadable;j++,samplestoread--)
						{
							x->outshort[writeoffset++] += fshort[(*readoffset)++] * x->prescale;
						}
						(*readoffset)%=x->ac[i]->channelsamples;
						if (!(*readoffset))
						{
							(*front)++;
							(*front)%=x->ac[i]->channelbuffers;
							//pthread_cond_signal(x->xhighcond); // Should wake up *one* thread
							pthread_cond_broadcast(x->xhighcond); // Should wake up *all* threads
						}
					}
				}
			}
		}
	}
	pthread_mutex_unlock(x->xmutex);
	return(x->mixerstatus);
}

float getdelay_audiomixer(audiomixer *x)
{
	return(((float)x->outbufferframes / (float)x->rate) * 1000.0);
}

void signalstop_audiomixer(audiomixer *x)
{
	pthread_mutex_lock(x->xmutex);
	x->mixerstatus = MX_STOPPED;
	pthread_cond_signal(x->xlowcond); // Should wake up *one* thread
	pthread_mutex_unlock(x->xmutex);
}

void close_audiomixer(audiomixer *x)
{
	pthread_mutex_lock(x->xmutex);
	while (x->activechannels)
		pthread_cond_wait(x->xchannelcond, x->xmutex);
	x->mixerstatus = MX_STOPPED;
	pthread_mutex_unlock(x->xmutex);

	free(x->ac);
	free(x->outbuffer);

	pthread_mutex_destroy(x->xmutex);
	free(x->xmutex);
	pthread_cond_destroy(x->xlowcond);
	free(x->xlowcond);
	pthread_cond_destroy(x->xhighcond);
	free(x->xhighcond);
	pthread_cond_destroy(x->xchannelcond);
	free(x->xchannelcond);
//printf("close_audiomixer\n");
}

void connect_audiojack(int channelbuffers, audiojack *aj, audiomixer *x)
{
	aj->x = x;
	aj->mxchannel = -1;
	aj->channelbuffers = channelbuffers;
}

void init_audiojack(int channelbuffers, int buffersize, audiojack *aj)
{
//printf("init_audiojack\n");
	aj->jackbuffersize = buffersize;
	aj->jacksamples = aj->jackbuffersize / ( snd_pcm_format_width(aj->x->format) / 8 );
	aj->jackframes = aj->jacksamples / aj->x->channels;
	aj->channelbuffers = channelbuffers;
	aj->mxchannel = allocatechannel_audiomixer(aj->jackframes, aj->channelbuffers, aj->x);
	aj->channelbuffer = aj->x->ac[aj->mxchannel]->channelbuffer;
	aj->rear = &(aj->x->ac[aj->mxchannel]->rear);
	aj->front = &(aj->x->ac[aj->mxchannel]->front);
//printf("init_audiojack %d buffersize %d\n", aj->mxchannel, buffersize);
}

void writetojack(char* inbuffer, int inbuffersize, audiojack *aj)
{
	pthread_mutex_lock(aj->x->xmutex);
	if (aj->mxchannel==-1)
		init_audiojack(aj->channelbuffers, inbuffersize, aj);
	if (aj->mxchannel!=-1)
	{
//if (aj->mxchannel==1) printf("channel buffersize %d\n", aj->x->ac[aj->mxchannel]->channelbuffersize);
		while ((*(aj->rear)+1)%aj->channelbuffers == *(aj->front)) // queue full
		{
//printf("sleeping, jack queue full, channel %d\n", aj->mxchannel);
			pthread_cond_wait(aj->x->xhighcond, aj->x->xmutex);
		}
//if (aj->mxchannel==1) printf("writing to %x, rear %d, buffersize %d\n", aj->channelbuffer[*(aj->rear)], *(aj->rear), inbuffersize);
		memcpy(aj->channelbuffer[*(aj->rear)], inbuffer, inbuffersize);
//printf("write done\n");
		(*(aj->rear))++;
		(*(aj->rear))%=aj->channelbuffers;
		pthread_cond_signal(aj->x->xlowcond); // Should wake up *one* thread
//printf("written to jack, channel %d, front %d rear %d\n", aj->mxchannel, (*(aj->front)), (*(aj->rear)));
	}
	else
		printf("init_audiojack can not allocate channel\n");
	pthread_mutex_unlock(aj->x->xmutex);
}

void close_audiojack(audiojack *aj)
{
//printf("close_audiojack %d\n", aj->mxchannel);
	if (aj->mxchannel!=-1)
	{
		pthread_mutex_lock(aj->x->xmutex);
		deallocatechannel_audiomixer(aj->mxchannel, aj->x);
		aj->mxchannel = -1;
		pthread_mutex_unlock(aj->x->xmutex);
	}
}

/*
void jack_initialize(int channelbuffers, audiojack *aj)
{
	aj->mxchannel = -1;
	aj->channelbuffers = channelbuffers;
}
*/
