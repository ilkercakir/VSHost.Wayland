#ifndef AudioEncoder_h
#define AudioEncoder_h

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

#include <alsa/asoundlib.h>

#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"

typedef enum{
	RS_IDLE = 0,
	RS_RECORDING
}recordingstate;

typedef struct
{
	snd_pcm_format_t format; // SND_PCM_FORMAT_S16
	unsigned int rate; // sampling rate
	unsigned int channels; // channels

	enum AVSampleFormat avformat; // AV_SAMPLE_FMT_S16
	int64_t bit_rate;

	enum AVCodecID id;
	AVCodec *codec;
	AVCodecContext *codeccontext;
	AVFormatContext *formatcontext;
	AVStream *stream;
	AVIOContext *iocontext;

	char *codecbuffer, *encoderbuffer;
	int encoderbuffersize, front, rear;
	int codecframesize;
	AVFrame *frame;
	int64_t pts;
	char *recordedfilename;

	recordingstate rstate;
	pthread_mutex_t recordmutex;
	pthread_cond_t recordqlowcond;
	pthread_cond_t recordqhighcond;
	pthread_t tid;
	int retval_thread;
}audioencoder;

void init_encoder(audioencoder *aen);
int start_encoder(audioencoder *aen, char *filename, enum AVCodecID id, enum AVSampleFormat avformat, snd_pcm_format_t format, unsigned int rate, unsigned int channels, int64_t bit_rate);
void encoder_add_buffer(audioencoder *aen, char *inbuffer, int buffersize);
void stop_encoder(audioencoder *aen);
void close_encoder(audioencoder *aen);
recordingstate encoder_getstate(audioencoder *aen);
#endif
