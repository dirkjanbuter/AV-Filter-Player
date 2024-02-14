#include "videodecoder.h"

CRESULT videodecoder_create(VIDEODECODER *v, TCHAR *filename)
{
    int ret;
    unsigned int i;
    
    v->stream_index = 0;
    v->stream_mapping = NULL;
    v->stream_mapping_size = 0;
	
    v->ifmt_ctx = 0;
    
    if ((ret = avformat_open_input(&v->ifmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return CFAILED;
    }

    if ((ret = avformat_find_stream_info(v->ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return CFAILED;
    }



    v->stream_mapping_size = v->ifmt_ctx->nb_streams;
    v->stream_mapping = av_mallocz_array(v->stream_mapping_size, sizeof(*v->stream_mapping));
    if (!v->stream_mapping) {
        ret = AVERROR(ENOMEM);
        return CFAILED;
    }

    v->dec_ctx = av_mallocz_array(v->ifmt_ctx->nb_streams, sizeof(*v->dec_ctx));
    if (!v->dec_ctx)
        return CFAILED;

    for (i = 0; i < v->ifmt_ctx->nb_streams; i++) {
        AVStream *stream = v->ifmt_ctx->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;

        if (codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            v->stream_mapping[i] = -1;
            continue;
        }

        AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx;
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            return CFAILED;
        }
        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            return CFAILED;
        }
        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                   "for stream #%u\n", i);
            return CFAILED;
        }
        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
                codec_ctx->framerate = av_guess_frame_rate(v->ifmt_ctx, stream, NULL);
            /* Open decoder */
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return CFAILED;
            }
        }
        v->dec_ctx[i] = codec_ctx;
        v->stream_mapping[i] = v->stream_index++;
    }

    v->frame = av_frame_alloc();
    if (!v->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return CFAILED;
    }

    av_dump_format(v->ifmt_ctx, 0, filename, 0);

    return CSUCCESS;
}

CRESULT videodecoder_destroy(VIDEODECODER *v)
{
    int i;

    for (i = 0; i < v->ifmt_ctx->nb_streams; i++) {
        if(i >= v->stream_mapping_size || v->stream_mapping[i] < 0)
            continue;
        avcodec_free_context(&v->dec_ctx[v->stream_mapping[i]]);
    }
    av_frame_free(&v->frame);
    avformat_close_input(&v->ifmt_ctx);
    av_freep(v->dec_ctx);
    av_freep(v->stream_mapping);
    return CSUCCESS;
}


AVFrame *videodecoder_getframe(VIDEODECODER *v)
{
    int ret = 0;
    int got_frame = 0;
    int i;

    for(;;) {
        AVStream *in_stream;
       
        ret = av_read_frame(v->ifmt_ctx, &v->pkt);
        if (ret < 0) {
                    break;
                    
                    ret = av_seek_frame(v->ifmt_ctx, v->pkt.stream_index, 0, AVSEEK_FLAG_ANY);
                    E("Seeking to 0");
                    if (ret < 0) {
                        E("Error: Error in seeking to begin!");
                        break;
                    }
                    continue;
        }

        in_stream  = v->ifmt_ctx->streams[v->pkt.stream_index];
        if(v->pkt.stream_index >= v->stream_mapping_size || v->stream_mapping[v->pkt.stream_index] < 0)
        {
            av_packet_unref(&v->pkt);
            continue;
        }

        AVCodecContext *c = v->dec_ctx[v->stream_mapping[v->pkt.stream_index]];

        ret = avcodec_send_packet(c, &v->pkt);
        if (ret < 0) {
            av_packet_unref(&v->pkt);
            fprintf(stderr, "Error sending a packet for decoding\n");
            continue;
        }

        ret = avcodec_receive_frame(c, v->frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_unref(&v->pkt);
            break;
        }
        else if (ret < 0) {
            av_packet_unref(&v->pkt);
            fprintf(stderr, "Error during decoding\n");
            continue;
        }
        av_packet_unref(&v->pkt);
        return v->frame;
    }
    return NULL;
}
