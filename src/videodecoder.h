#ifndef VIDEODECODER_H_INCLUDED
#define VIDEODECODER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/time.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "common.h"
#include "log.h"

typedef struct VIDEODECODER {
    AVFrame *frame;
    AVFormatContext *ifmt_ctx;
    AVCodecContext **dec_ctx;
    int stream_index;
    int *stream_mapping;
    int stream_mapping_size;
    AVPacket pkt;
} VIDEODECODER;

CRESULT videodecoder_create(VIDEODECODER *v, TCHAR *filename);
CRESULT videodecoder_destroy(VIDEODECODER *v);
AVFrame *videodecoder_getframe(VIDEODECODER *v);

#endif
