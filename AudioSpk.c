/*
 * AudioSpk.c
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

#include "AudioSpk.h"

int set_hwparams(speaker *s)
{
	unsigned int rrate;
	snd_pcm_uframes_t size;
	int err, dir;

	/* choose all parameters */
	if ((err = snd_pcm_hw_params_any(s->handle, s->hwparams)) < 0)
	{
		printf("Broken configuration for playback: no configurations available: %s\n", snd_strerror(err));
		return(err);
	}

	/* set hardware resampling */
	if ((err = snd_pcm_hw_params_set_rate_resample(s->handle, s->hwparams, s->resample)) < 0)
	{
		printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
		return(err);
	}

	/* set the interleaved read/write format */
	
	if ((err = snd_pcm_hw_params_set_access(s->handle, s->hwparams, s->access)) < 0)
	{
		printf("Access type not available for playback: %s\n", snd_strerror(err));
		return(err);
	}

	/* set the sample format */
	if ((err = snd_pcm_hw_params_set_format(s->handle, s->hwparams, s->format)) < 0)
	{
		printf("Sample format not available for playback: %s\n", snd_strerror(err));
		return(err);
	}

	/* set the count of channels */
	if ((err = snd_pcm_hw_params_set_channels(s->handle, s->hwparams, s->channels)) < 0)
	{
		printf("Channels count (%i) not available for playbacks: %s\n", s->channels, snd_strerror(err));
		return(err);
	}

	/* set the stream rate */
	rrate = s->rate;
	if ((err = snd_pcm_hw_params_set_rate_near(s->handle, s->hwparams, &rrate, 0)) < 0)
	{
		printf("Rate %iHz not available for playback: %s\n", s->rate, snd_strerror(err));
		return(err);
	}
	if (rrate != s->rate)
	{
		printf("Rate doesn't match (requested %iHz, get %iHz)\n", s->rate, rrate);
		return(-EINVAL);
	}

	if ((err = snd_pcm_hw_params_set_buffer_size(s->handle, s->hwparams, s->bufsize)) < 0)
	{
		printf("Unable to set buffer size %d for playback: %s\n", s->bufsize, snd_strerror(err));
		return err;
	}

	if ((err = snd_pcm_hw_params_get_buffer_size(s->hwparams, &size)) < 0)
	{
		printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
		return(err);
	}
	s->buffer_size = size;

	//if ((err = snd_pcm_hw_params_set_period_size(s->handle, s->hwparams, s->persize, 0)) < 0)
	if ((err = snd_pcm_hw_params_set_period_size_near(s->handle, s->hwparams, &s->persize, &dir)) < 0)
	{
		printf("Unable to set period size %ld for playback: %s\n", s->persize, snd_strerror(err));
		return(err);
	}

	if ((err = snd_pcm_hw_params_get_period_size(s->hwparams, &size, &dir)) < 0)
	{
		printf("Unable to get period size for playback: %s\n", snd_strerror(err));
		return err;
	}
	s->period_size = size;

	/* write the parameters to device */
	if ((err = snd_pcm_hw_params(s->handle, s->hwparams)) < 0)
	{
		printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
		return err;
	}
	return(0);
}

int set_swparams(speaker *s)
{
	int err;

	/* get the current swparams */
	if ((err = snd_pcm_sw_params_current(s->handle, s->swparams)) < 0)
	{
		printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
		return(err);
	}

	/* start the transfer when the buffer is almost full: */
	/* (buffer_size / avail_min) * avail_min */
	//err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (buffer_size / period_size) * period_size);

	// start transfer when first period arrives
	if ((err = snd_pcm_sw_params_set_start_threshold(s->handle, s->swparams, s->period_size)) < 0)
	{
		printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
		return(err);
	}

	/* allow the transfer when at least period_size samples can be processed */
	/* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
	if ((err = snd_pcm_sw_params_set_avail_min(s->handle, s->swparams, s->period_event ? s->buffer_size : s->period_size)) < 0)
	{
		printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
		return(err);
	}

	/* enable period events when requested */
	if (s->period_event)
	{
		if ((err = snd_pcm_sw_params_set_period_event(s->handle, s->swparams, 1)) < 0)
		{
			printf("Unable to set period event: %s\n", snd_strerror(err));
			return(err);
		}
	}

	/* write the parameters to the playback device */
	if ((err = snd_pcm_sw_params(s->handle, s->swparams)) < 0)
	{
		printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
		return(err);
	}

	return(0);
}

int init_audio_hw_spk(speaker *s)
{
	int err;

	s->persize = 1024; // frames
	//s->bufsize = 10240; // persize * 10; // 10 periods
	s->bufsize = s->persize * 3; // 3 periods

	snd_pcm_hw_params_alloca(&(s->hwparams));
	snd_pcm_sw_params_alloca(&(s->swparams));

	if ((err = snd_pcm_open(&(s->handle), s->device, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
	{
		printf("Playback open error: %s\n", snd_strerror(err));
		return(err);
	}
	if ((err = set_hwparams(s)) < 0)
	{
		printf("Setting of hwparams failed: %s\n", snd_strerror(err));
		return(err);
	}
	if ((err = set_swparams(s)) < 0)
	{
		printf("Setting of swparams failed: %s\n", snd_strerror(err));
		return(err);
	}

	return(0);
}

void close_audio_hw_spk(speaker *s)
{
	snd_pcm_close(s->handle);
}

static int xrun_recovery(snd_pcm_t *handle, int err) // Underrun and suspend recovery
{
	if (err == -EPIPE)	// under-run
	{
		err = snd_pcm_prepare(handle);
		if (err < 0)
			printf("Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
		return(0);
	}
	else if (err == -ESTRPIPE)
	{
		while ((err = snd_pcm_resume(handle)) == -EAGAIN)
			sleep(1);	/* wait until the suspend flag is released */
		if (err < 0)
		{
			err = snd_pcm_prepare(handle);
			if (err < 0)
				printf("Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
		}
		return(0);
	}
	return(err);
}

void init_spk(speaker *s, char* device, snd_pcm_format_t format, unsigned int rate, unsigned int channels)
{
	strcpy(s->device, device);
	s->rate = rate;
	s->format = format;
	s->channels = channels;
	s->resample = 1;
	s->period_event = 0;
	s->access = SND_PCM_ACCESS_RW_INTERLEAVED;
}

void write_spk(speaker *s, char* buffer, int bufferframes)
{
	int err;

	err = snd_pcm_avail_delay(s->handle, &(s->availp), &(s->delayp));
	//printf("\t%d\t%d\n", (int)s->availp, (int)s->delayp);

	err = snd_pcm_writei(s->handle, buffer, bufferframes);
	if (err == -EAGAIN) printf("EAGAIN\n");
	if (err < 0)
	{
		if (xrun_recovery(s->handle, err) < 0)
		{
			printf("snd_pcm_writei error: %s\n", snd_strerror(err));
		}
	}
}

void close_spk(speaker *s)
{
}
