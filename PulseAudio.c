/*
 * PulseAudio.c
 * 
 * Copyright 2021  <pi@raspberrypi>
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

#include "PulseAudio.h"

void quit_pamain(paplayer *p, int ret)
{
	assert(p->mainloop_api);
	p->mainloop_api->quit(p->mainloop_api, ret);
}

/* Connection draining complete */
void context_drain_complete(pa_context *c, void *userdata)
{
	pa_context_disconnect(c);
}

/* Stream draining complete */
void stream_drain_complete(pa_stream *s, int success, void *userdata) 
{
	pa_operation *o;
	paplayer *p = (paplayer *)userdata;

	if (!success)
	{
		printf("Failed to drain stream: %s\n", pa_strerror(pa_context_errno(p->context)));
		quit_pamain(p, 1);
	}

	if (p->verbose)
		printf("Playback stream drained.\n");

	pa_stream_disconnect(p->stream);
	pa_stream_unref(p->stream);
	p->stream = NULL;

	if (!(o = pa_context_drain(p->context, context_drain_complete, (void *)p)))
		pa_context_disconnect(p->context);
	else
	{
		pa_operation_unref(o);

		if (p->verbose)
			printf("Draining connection to server.\n");
	}
}

/* Start draining */
static void start_drain(paplayer *p)
{
    if (p->stream)
    {
        pa_operation *o;

		pa_stream_set_write_callback(p->stream, NULL, NULL);

        if (!(o = pa_stream_drain(p->stream, stream_drain_complete, p)))
        {
            printf("pa_stream_drain(): %s", pa_strerror(pa_context_errno(p->context)));
            quit_pamain(p, 1);
            return;
        }

        pa_operation_unref(o);
    } else
        quit_pamain(p, 0);
}

/* This is called whenever new data may be written to the stream */
void stream_write_callback(pa_stream *s, size_t length, void *userdata)
{
	paplayer *p = (paplayer *)userdata;
	void *data;
	int ret;

	if (p->status == PA_IDLE) return;

	if (p->verbose) printf("write %d\n", (int)length);

	if (p->status == PA_STOPPED)
	{
		p->status = PA_IDLE;
		if (p->verbose) printf("closed\n");
	}
	else
	{
		data = pa_xmalloc(length);
		paplayer_remove(p, data, length);
		if ((ret=pa_stream_write(s, data, length, pa_xfree, 0, PA_SEEK_RELATIVE))<0)
		{
			printf("Write errror: %d %s\n", ret, pa_strerror(pa_context_errno(pa_stream_get_context(s))));
		}
	}
}

/* This is called whenever new data is available for reading*/
void stream_read_callback(pa_stream *s, size_t length, void *userdata) {
	paplayer *p = (paplayer *)userdata;
	void *data;
	int ret;

	if (p->status == PA_IDLE) return;

	if (p->verbose) printf("read %d\n", (int)length);

	if (p->status == PA_STOPPED)
	{
		p->status = PA_IDLE;
		if (p->verbose) printf("closed\n");
	}
	else
	{
		if ((ret=pa_stream_peek(s, &data, &length)) < 0)
		{
			printf("Read errror: %d %s\n", ret, pa_strerror(pa_context_errno(pa_stream_get_context(s))));
		}
		else
		{
			if (!data)
			{
				if (!length) // no data
				{}
				else // hole
					pa_stream_drop(s);
			}
			else
			{
				paplayer_add(p, data, length);
				pa_stream_drop(s);
			}
		}
	}
}

/* This routine is called whenever the stream state changes */
void stream_state_callback(pa_stream *s, void *userdata)
{
	paplayer *p = (paplayer *)userdata;

	assert(s);

	switch (pa_stream_get_state(s))
	{
		case PA_STREAM_CREATING:
		case PA_STREAM_TERMINATED:
			break;
		case PA_STREAM_READY:
			if (p->verbose)
				printf("Stream successfully created\n");
			break;
		case PA_STREAM_FAILED:
		default:
			printf("Stream errror: %s\n", pa_strerror(pa_context_errno(pa_stream_get_context(s))));
			quit_pamain(p, 1);
	}
}

/* This is called whenever the context status changes */
void context_state_callback(pa_context *c, void *userdata)
{
	int ret;
	paplayer *p = (paplayer *)userdata;

	assert(c);

	switch (pa_context_get_state(c)) 
	{
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			break;
		case PA_CONTEXT_READY: 
            assert(c);
            assert(!p->stream);

			if (p->verbose)
				printf("Connection established.\n");

			p->stream = pa_stream_new(c, p->stream_name, &(p->sample_spec), p->channel_map_set ? &(p->channel_map) : NULL);
			assert(p->stream);

			pa_stream_set_state_callback(p->stream, stream_state_callback, userdata);
			pa_stream_set_write_callback(p->stream, stream_write_callback, userdata);
			pa_stream_set_read_callback(p->stream, stream_read_callback, userdata);

			p->bufattr.fragsize = pa_usec_to_bytes(p->latency,&(p->sample_spec)); //(uint32_t) -1;
			p->bufattr.maxlength = pa_usec_to_bytes(p->latency,&(p->sample_spec)); // (uint32_t) -1;
			p->bufattr.minreq = pa_usec_to_bytes(p->latency,&(p->sample_spec)); //(uint32_t) -1;
			p->bufattr.prebuf = 0; //(uint32_t) -1;
			p->bufattr.tlength = pa_usec_to_bytes(p->latency,&(p->sample_spec));

			p->stream_flags = PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_NOT_MONOTONIC | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_ADJUST_LATENCY;

			if (p->mode == PLAYBACK)
			{
				if ((ret = pa_stream_connect_playback(p->stream, p->device, &(p->bufattr), p->stream_flags, NULL, NULL))<0)
					printf("pa_stream_connect_playback() failed (%d): %s\n", ret, pa_strerror(pa_context_errno(c)));
			}
			else
			{
				if ((ret = pa_stream_connect_record(p->stream, p->device, &(p->bufattr), p->stream_flags))<0)
					printf("pa_stream_connect_record() failed (%d): %s\n", ret, pa_strerror(pa_context_errno(c)));
			}
			break;
		case PA_CONTEXT_TERMINATED:
			quit_pamain(p, 0);
			break;
		case PA_CONTEXT_FAILED:
		default:
			printf("Connection failure: %s\n", pa_strerror(pa_context_errno(c)));
			quit_pamain(p, 1);
	}
}

gpointer pulseaudio_thread(gpointer args)
{
	paplayer *p = (paplayer *)args;

	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	/* Set up a new main loop */
	if (!(p->m = pa_mainloop_new()))
	{
		printf("pa_mainloop_new() failed.\n");
	}
	else
	{
		p->mainloop_api = pa_mainloop_get_api(p->m);

		/* Create a new connection context */
		if (!(p->context = pa_context_new(p->mainloop_api, p->client_name)))
		{
			printf("pa_context_new() failed.\n");
		}
		else
		{
			pa_context_set_state_callback(p->context, context_state_callback, (void *)p);

			/* Connect the context */
			if (pa_context_connect(p->context, p->server, 0, NULL) < 0) 
			{
				printf("pa_context_connect() failed: %s\n", pa_strerror(pa_context_errno(p->context)));
			}
			else
			{
				/* Run the main loop */
				int ret = 1;
				if (pa_mainloop_run(p->m, &ret) < 0)
				{
					printf("pa_mainloop_run() failed.\n");
				}
				if (p->stream)
					pa_stream_unref(p->stream);

				if (p->context)
					pa_context_unref(p->context);

				if (p->m)
				{
					pa_signal_done();
					pa_mainloop_free(p->m);
				}
			}
		}
	}

	p->retval = 0;
	pthread_exit(&(p->retval));
}

void init_paplayer(paplayer *p, snd_pcm_format_t format, unsigned int rate, unsigned int channels)
{
	int err;

	p->context = NULL;
	p->stream = NULL;
	p->mainloop_api = NULL;
	p->stream_name = "Audio Stream";
	p->client_name = "Virtual Studio Host";
	p->device = NULL;
	p->server = NULL;
	p->verbose = 0;
	//p->volume = PA_VOLUME_NORM;
	switch (format)
	{
		case SND_PCM_FORMAT_S16:
		default:
			p->sample_spec.format = PA_SAMPLE_S16LE;
			break;
	}
	p->sample_spec.rate = (uint32_t)rate;
	p->sample_spec.channels = (uint8_t)channels;
	assert(pa_sample_spec_valid(&(p->sample_spec)));
	p->channel_map_set = 0;
	p->latency = 10000; // start latency in micro seconds
	p->name = "VSHost";
	p->status = PA_RUNNING;
	p->mode = PLAYBACK;

	audioCQ_init(&(p->ap), format, rate, channels, 1024, 6*1024);

	err = pthread_create(&(p->tid), NULL, &pulseaudio_thread, (void*)p);
	if (err)
	{}
//printf("thread %s\n", ao->name);
	CPU_ZERO(&(p->cpu));
	CPU_SET(1, &(p->cpu));
	if ((err=pthread_setaffinity_np(p->tid, sizeof(cpu_set_t), &(p->cpu))))
	{
		//printf("pthread_setaffinity_np error %d\n", err);
	}
}

void init_parecorder(paplayer *p, snd_pcm_format_t format, unsigned int rate, unsigned int channels)
{
	int err;

	p->context = NULL;
	p->stream = NULL;
	p->mainloop_api = NULL;
	p->stream_name = "Audio Stream";
	p->client_name = "Virtual Studio Host";
	p->device = NULL;
	p->server = NULL;
	p->verbose = 0;
	//p->volume = PA_VOLUME_NORM;
	switch (format)
	{
		case SND_PCM_FORMAT_S16:
		default:
			p->sample_spec.format = PA_SAMPLE_S16LE;
			break;
	}
	p->sample_spec.rate = (uint32_t)rate;
	p->sample_spec.channels = (uint8_t)channels;
	assert(pa_sample_spec_valid(&(p->sample_spec)));
	p->channel_map_set = 0;
	p->latency = 10000; // start latency in micro seconds
	p->name = "VSHost";
	p->status = PA_RUNNING;
	p->mode = RECORD;

	audioCQ_init(&(p->ap), format, rate, channels, 1024, 6*1024);

	err = pthread_create(&(p->tid), NULL, &pulseaudio_thread, (void*)p);
	if (err)
	{}
//printf("thread %s\n", ao->name);
	CPU_ZERO(&(p->cpu));
	CPU_SET(1, &(p->cpu));
	if ((err=pthread_setaffinity_np(p->tid, sizeof(cpu_set_t), &(p->cpu))))
	{
		//printf("pthread_setaffinity_np error %d\n", err);
	}
}

void paplayer_add(paplayer *p, char *inbuffer, int inbuffersize)
{
	audioCQ_add(&(p->ap), inbuffer, inbuffersize);
}

void paplayer_remove(paplayer *p, char *outbuffer, int outbuffersize)
{
	audioCQ_removeVB(&(p->ap), outbuffer, outbuffersize);
}

void close_paplayer(paplayer *p)
{
	int i;

	if (p->mode == PLAYBACK)
		start_drain(p);
	else
	{
		stream_drain_complete(p->stream, 1, (void *)p);
	}

	p->status = PA_STOPPED;

	if ((i=pthread_join(p->tid, NULL)))
		printf("pthread_join error, %s, %d\n", p->name, i);
}
