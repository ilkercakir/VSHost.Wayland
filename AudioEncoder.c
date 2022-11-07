#include "AudioEncoder.h"

int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE)
    {
//printf("%d\n", *p);
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

int select_sample_rate(AVCodec *codec)
{
	const int *p;

	p = codec->supported_samplerates;
    while (*p)
    {
//printf("%d\n", *p);
        p++;
    }
    return 0;
}

static void* encoder_thread(void* args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	audioencoder *aen = (audioencoder *)args;

	AVPacket pkt;
	int ret = 0, data_present, i, j, frame_size;
	int offset, bytesleft, bytestocopy;

	pthread_mutex_lock(&(aen->recordmutex));
	aen->rstate = RS_RECORDING;
	pthread_mutex_unlock(&(aen->recordmutex));

	// read cq
	while(1)
	{
		pthread_mutex_lock(&(aen->recordmutex));
		while (((aen->rear - aen->front + aen->encoderbuffersize)%aen->encoderbuffersize < aen->codecframesize) && (aen->rstate == RS_RECORDING))
		{
			//printf("encoder queue sleeping, underrun\n");
			pthread_cond_wait(&(aen->recordqlowcond), &(aen->recordmutex));
		}

		if (aen->rstate != RS_RECORDING)
		{
			pthread_mutex_unlock(&(aen->recordmutex));
			break;
		}

//printf("read %d / %d\n", aen->codecframesize, aen->frame->linesize[0]);
		for(offset=0,bytesleft=aen->codecframesize;bytesleft;bytesleft-=bytestocopy)
		{
			bytestocopy = (aen->front+bytesleft>aen->encoderbuffersize?aen->encoderbuffersize-aen->front:bytesleft);
			memcpy(aen->codecbuffer+offset, aen->encoderbuffer+aen->front, bytestocopy);
			offset += bytestocopy;
			aen->front += bytestocopy; aen->front %= aen->encoderbuffersize;
		}
		pthread_cond_signal(&(aen->recordqhighcond)); // Should wake up *one* thread
		pthread_mutex_unlock(&(aen->recordmutex));

		// make sure the frame is writable, makes a copy if the encoder kept a reference internally
		if ((ret = av_frame_make_writable(aen->frame))<0)
		{
			printf("Could not make frame writable (error '%s')\n", av_err2str(ret));
			ret = -1;
		}
		else
		{
			if (av_sample_fmt_is_planar(aen->avformat))
			{
				frame_size = aen->codeccontext->frame_size;
				signed short *src = (signed short *)aen->codecbuffer;
				switch (aen->avformat)
				{
					case AV_SAMPLE_FMT_S16P:
						for(i=0;i<aen->channels;i++)
						{
							signed short *dst = (signed short *)aen->frame->data[i];
							for(j=0;j<frame_size;j++)
							{
								dst[j] = src[j*aen->channels+i];
							}
						}
						break;
					case AV_SAMPLE_FMT_FLTP:
						for(i=0;i<aen->channels;i++)
						{
							float *dstf = (float *)aen->frame->data[i];
							for(j=0;j<frame_size;j++)
							{
								dstf[j] = (float)src[j*aen->channels+i] / 32767.0f;
							}
						}
						break;
					default: break;
				}
			}
			else
			{
				switch (aen->avformat)
				{
					case AV_SAMPLE_FMT_S16:
						memcpy(aen->frame->data[0], aen->codecbuffer, aen->codecframesize);
						break;
					case AV_SAMPLE_FMT_FLT:
						i = 0;
						signed short *srci = (signed short *)aen->codecbuffer;
						float *dsti = (float *)aen->frame->data[0];
						for(i=0;i<aen->codecframesize;i++)
							dsti[i] = (float)srci[i];
						break;
					default: break;
				}
			}

			av_init_packet(&pkt);
			pkt.data = NULL; // packet data will be allocated by the encoder
			pkt.size = 0;

			// Set a timestamp based on the sample rate for the container.
			aen->frame->pts = aen->pts;
			aen->pts += aen->frame->nb_samples;

			if ((ret = avcodec_encode_audio2(aen->codeccontext, &pkt, aen->frame, &data_present)) < 0)
			{
				printf("Could not encode frame (error '%s')\n", av_err2str(ret));
				av_packet_unref(&pkt);
				ret = -1;
			}
			else
			{
				// Write one audio frame from the temporary packet to the output file.
				if (data_present) 
				{
					if ((ret = av_write_frame(aen->formatcontext, &pkt)) < 0)
					{
						printf("Could not write frame (error '%s')\n", av_err2str(ret));
						av_packet_unref(&pkt);
						ret = -1;
					}
					else
						av_packet_unref(&pkt);
				}
			}
		}

		if (ret<0) break;
	}

	if ((ret = av_write_trailer(aen->formatcontext)) < 0)
	{
		printf("Could not write output file trailer (error '%s')\n", av_err2str(ret));
		ret = -1;
	}

//printf("exiting encoder_thread\n");
	aen->retval_thread = ret;
	pthread_exit(&(aen->retval_thread));
}

void encoder_create_thread(audioencoder *aen)
{
	int err;
	err = pthread_create(&(aen->tid), NULL, &encoder_thread, (void*)aen);
	if (err)
	{}
}

void encoder_terminate_thread(audioencoder *aen)
{
	pthread_mutex_lock(&(aen->recordmutex));
	aen->rstate = RS_IDLE;
	pthread_cond_signal(&(aen->recordqlowcond)); // Should wake up *one* thread
	pthread_mutex_unlock(&(aen->recordmutex));

	int i;
	if ((i=pthread_join(aen->tid, NULL)))
		printf("pthread_join error, aen->tid, %d\n", i);
}

void init_encoder(audioencoder *aen)
{
	int err;

	aen->rstate = RS_IDLE;

	if((err=pthread_mutex_init(&(aen->recordmutex), NULL))!=0 )
		printf("record mutex init failed, %d\n", err);

	if ((err=pthread_cond_init(&(aen->recordqlowcond), NULL))!=0 )
		printf("record queue low init failed, %d\n", err);

	if ((err=pthread_cond_init(&(aen->recordqhighcond), NULL))!=0 )
		printf("record queue low init failed, %d\n", err);
}

int start_encoder(audioencoder *aen, char *filename, enum AVCodecID id, enum AVSampleFormat avformat, snd_pcm_format_t format, unsigned int rate, unsigned int channels, int64_t bit_rate)
{
	int err;

	aen->format = format;
	aen->rate = rate;
	aen->channels = channels;
	aen->bit_rate = bit_rate;

	aen->avformat = avformat;
	aen->id = id;
	aen->codec = NULL;
	aen->codeccontext = NULL;
	aen->formatcontext = NULL;
	aen->stream = NULL;
	aen->iocontext = NULL;
	aen->pts = 0;

	aen->encoderbuffer = NULL;
	aen->front = aen->rear = 0;

	/* register all the codecs */
	av_register_all();
	avcodec_register_all();

	/* Open the output file to write to it. */
	if ((err = avio_open(&(aen->iocontext), filename, AVIO_FLAG_WRITE)) < 0)
	{
		printf("Could not open output file '%s' (err '%s')\n", filename, av_err2str(err));
		return -1;
	}

	/* Create a new format context for the output container format. */
	if (!(aen->formatcontext = avformat_alloc_context()))
	{
		printf("Could not allocate output format context\n");
        return -1;
	}

	err = 0;

	/* Associate the output file (pointer) with the container format context. */
	aen->formatcontext->pb = aen->iocontext;

	/* Guess the desired container format based on the file extension. */
	if (!(aen->formatcontext->oformat = av_guess_format(NULL, filename, NULL)))
	{
		printf("Could not find output file format %s\n", filename);
		err = -1;
	}
	else
	{
		av_strlcpy(aen->formatcontext->filename, filename, sizeof(aen->formatcontext->filename));

		/* Find the encoder to be used by its name. */
		aen->codec = avcodec_find_encoder(aen->id);
		if (!aen->codec)
		{
			printf("Codec %d not found\n", aen->id);
			err = -1;
		}
		else
		{
			//check_sample_fmt(aen->codec, aen->avformat);

			/* Create a new audio stream in the output file container. */
			if (!(aen->stream = avformat_new_stream(aen->formatcontext, NULL)))
			{
				printf("Could not create new stream\n");
				err = -1;
			}
			else
			{
				aen->codeccontext = avcodec_alloc_context3(aen->codec);
				if (!aen->codeccontext)
				{
					printf("Could not allocate audio codec context\n");
					err = -1;
				}
				else
				{
					//select_sample_rate(aen->codec);
					
					aen->codeccontext->channels = aen->channels;
					aen->codeccontext->channel_layout = av_get_default_channel_layout(aen->channels);
					aen->codeccontext->sample_rate = aen->rate;
					aen->codeccontext->sample_fmt = aen->avformat;
					aen->codeccontext->bit_rate = aen->bit_rate;

					/* Allow the use of the experimental AAC encoder */
					aen->codeccontext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

					/* Set the sample rate for the container. */
					aen->stream->time_base.den = aen->rate;
					aen->stream->time_base.num = 1;

					/*
					 * Some container formats (like MP4) require global headers to be present
					 * Mark the encoder so that it behaves accordingly.
					 */
					if (aen->formatcontext->oformat->flags & AVFMT_GLOBALHEADER)
						aen->codeccontext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

					/* Open the encoder for the audio stream to use it later. */
					if ((err = avcodec_open2(aen->codeccontext, aen->codec, NULL)) < 0)
					{
						printf("Could not open output codec (error '%s')\n", av_err2str(err));
						err = -1;
					}
					else
					{
						if ((err = avcodec_parameters_from_context(aen->stream->codecpar, aen->codeccontext)) < 0)
						{
							printf("Could not initialize stream parameters\n");
							err = -1;
						}
						else
						{
//printf("extradata_size %d\n", aen->codeccontext->extradata_size);
							if (!aen->stream->codecpar->extradata && aen->codeccontext->extradata)
							{
								aen->stream->codecpar->extradata = av_malloc(aen->codeccontext->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
								if (!aen->stream->codecpar->extradata)
								{
									printf("Could not allocate extradata buffer to copy parser data.\n");
								}
								else
								{ 
									aen->stream->codecpar->extradata_size = aen->codeccontext->extradata_size;
									memcpy(aen->stream->codecpar->extradata, aen->codeccontext->extradata, aen->codeccontext->extradata_size);
								}
							}

							/* Write the header of the output file container. */
							if ((err = avformat_write_header(aen->formatcontext, NULL)) < 0)
							{
								printf("Could not write output file header (error '%s')\n", av_err2str(err));
								err = -1;
							}
							else
							{
								/* frame containing input raw audio */
								aen->frame = av_frame_alloc();
								if (!(aen->frame))
								{
									printf("Could not allocate audio frame\n");
									err = -1;
								}
								else
								{
//printf("frame_size %d\n", aen->codeccontext->frame_size);
									aen->frame->nb_samples = aen->codeccontext->frame_size;
									aen->frame->channels = aen->codeccontext->channels;
									aen->frame->channel_layout = aen->codeccontext->channel_layout;
									aen->frame->format = aen->codeccontext->sample_fmt;
									aen->frame->sample_rate = aen->rate;
									/* allocate the data buffers */
									if ((err = av_frame_get_buffer(aen->frame, 0)) < 0)
									{
										printf("Could not allocate audio data buffers\n");
										err = -1;
									}
									else
									{
										aen->codecframesize = aen->codeccontext->frame_size * aen->channels * ( snd_pcm_format_width(format) / 8 );
										aen->codecbuffer = malloc(aen->codecframesize);
										aen->encoderbuffersize = aen->codecframesize * 10;
										aen->encoderbuffer = malloc(aen->encoderbuffersize);

										encoder_create_thread(aen);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (err)
	{
		avcodec_free_context(&(aen->codeccontext));
		avio_closep(&(aen->formatcontext)->pb);
		avformat_free_context(aen->formatcontext);
		aen->formatcontext = NULL;
	}

	return err;
}

void encoder_add_buffer(audioencoder *aen, char *inbuffer, int buffersize)
{
	int offset, bytesleft, bytestocopy;

	pthread_mutex_lock(&(aen->recordmutex));
	if (aen->rstate == RS_RECORDING)
	{
		while ((aen->front - aen->rear + aen->encoderbuffersize -1)%aen->encoderbuffersize < buffersize) // capacity < input
		{
			//printf("encoder queue sleeping, overrun\n");
			pthread_cond_wait(&(aen->recordqhighcond), &(aen->recordmutex));
		}

		for(offset=0,bytesleft=buffersize;bytesleft;bytesleft-=bytestocopy)
		{
			bytestocopy = (aen->rear+bytesleft>aen->encoderbuffersize?aen->encoderbuffersize-aen->rear:bytesleft);
			memcpy(aen->encoderbuffer+aen->rear, inbuffer+offset, bytestocopy);
			offset += bytestocopy;
			aen->rear += bytestocopy; aen->rear %= aen->encoderbuffersize;
		}

		pthread_cond_signal(&(aen->recordqlowcond)); // Should wake up *one* thread
//printf("encoder_add_buffer %d / %d\n", buffersize, aen->encoderbuffersize);
	}
	pthread_mutex_unlock(&(aen->recordmutex));
}

void stop_encoder(audioencoder *aen)
{
	encoder_terminate_thread(aen);

	if (aen->codeccontext)
	{
		avcodec_close(aen->codeccontext);
		avcodec_free_context(&(aen->codeccontext));
	}

	if (aen->formatcontext)
		avformat_free_context(aen->formatcontext);

	if (aen->iocontext)
		avio_close(aen->iocontext);

	free(aen->codecbuffer);
	free(aen->encoderbuffer);
}

void close_encoder(audioencoder *aen)
{
	if (aen->rstate != RS_IDLE)
		stop_encoder(aen);
		
	pthread_mutex_destroy(&(aen->recordmutex));
	pthread_cond_destroy(&(aen->recordqlowcond));
	pthread_cond_destroy(&(aen->recordqhighcond));
}

recordingstate encoder_getstate(audioencoder *aen)
{
	recordingstate ret;

	pthread_mutex_lock(&(aen->recordmutex));
	ret = aen->rstate;
	pthread_mutex_unlock(&(aen->recordmutex));
	return ret;
}
