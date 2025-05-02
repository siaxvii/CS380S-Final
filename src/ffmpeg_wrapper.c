#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include "ffmpeg_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

int read_packet(void* opaque, uint8_t* buf, int buf_size) {
    BufferData* bd = (BufferData*) opaque;
    size_t remaining = bd->size - bd->pos;
    if (remaining == 0)
        return AVERROR_EOF;
    int copy_size = buf_size < remaining ? buf_size : remaining;
    memcpy(buf, bd->data + bd->pos, copy_size);
    bd->pos += copy_size;
    return copy_size;
}

int sandboxed_decode_audio(const char* file_data, const size_t file_size, uint8_t** out_data, size_t* out_size) {
    AVFormatContext* fmt_ctx = NULL;
    AVIOContext* avio_ctx = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVCodec* decoder = NULL;
    AVPacket* packet = NULL;
    AVFrame* frame = NULL;
    SwrContext* swr_ctx = NULL;

    uint8_t* avio_ctx_buffer = NULL;
    size_t avio_ctx_buffer_size = 4096;
    int audio_stream_idx = -1;
    int ret = 0;

    uint8_t** output = NULL;
    int output_linesize = 0;
    int output_capacity = 0;

    uint8_t* final_data = NULL;
    int final_size = 0;
    int final_allocated = 0;

    fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
        return -1;
    }

    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    BufferData bd = { (const uint8_t*) file_data, (size_t) file_size, 0 };
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, &bd, read_packet, NULL, NULL);
    if (!avio_ctx) {
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    fmt_ctx->pb = avio_ctx;
    fmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if (ret != 0) goto cleanup;

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret != 0) goto cleanup;

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    if (audio_stream_idx == -1) {
        ret = -3;
        goto cleanup;
    }

    decoder = avcodec_find_decoder(fmt_ctx->streams[audio_stream_idx]->codecpar->codec_id);
    if (!decoder) {
        ret = -4;
        goto cleanup;
    }

    codec_ctx = avcodec_alloc_context3(decoder);
    if (!codec_ctx) {
        ret = -5;
        goto cleanup;
    }

    if (avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[audio_stream_idx]->codecpar) < 0) {
        ret = -6;
        goto cleanup;
    }

    if (avcodec_open2(codec_ctx, decoder, NULL) < 0) {
        ret = -7;
        goto cleanup;
    }

    packet = av_packet_alloc();
    frame = av_frame_alloc();
    if (!packet || !frame) {
        ret = -8;
        goto cleanup;
    }

    AVChannelLayout out_ch_layout;
    av_channel_layout_default(&out_ch_layout, 2);

    if (swr_alloc_set_opts2(
            &swr_ctx,
            &out_ch_layout,
            AV_SAMPLE_FMT_S16,
            44100,
            &codec_ctx->ch_layout,
            codec_ctx->sample_fmt,
            codec_ctx->sample_rate,
            0,
            NULL) < 0) {
        ret = -9;
        goto cleanup;
    }

    if (swr_init(swr_ctx) < 0) {
        ret = -9;
        goto cleanup;
    }

    while (av_read_frame(fmt_ctx, packet) == 0) {
        if (packet->stream_index == audio_stream_idx) {
            if (avcodec_send_packet(codec_ctx, packet) == 0) {
                while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    int out_samples = av_rescale_rnd(
                        swr_get_delay(swr_ctx, codec_ctx->sample_rate) + frame->nb_samples,
                        44100,
                        codec_ctx->sample_rate,
                        AV_ROUND_UP
                    );

                    if (out_samples > output_capacity) {
                        if (output) {
                            av_freep(&output[0]);
                            av_freep(&output);
                        }
                        if (av_samples_alloc_array_and_samples(
                                &output,
                                &output_linesize,
                                2,
                                out_samples,
                                AV_SAMPLE_FMT_S16,
                                0) < 0) {
                            ret = -10;
                            goto cleanup;
                        }
                        output_capacity = out_samples;
                    }

                    int converted_samples = swr_convert(
                        swr_ctx,
                        output,
                        out_samples,
                        (const uint8_t**)frame->data,
                        frame->nb_samples
                    );

                    if (converted_samples < 0) {
                        ret = -11;
                        goto cleanup;
                    }

                    int buffer_size = av_samples_get_buffer_size(
                        NULL, 2, converted_samples, AV_SAMPLE_FMT_S16, 1
                    );
                    if (buffer_size < 0) {
                        ret = -12;
                        goto cleanup;
                    }

                    if (final_size + buffer_size > final_allocated) {
                        int new_size = final_allocated == 0 ? 65536 : final_allocated * 2;
                        while (new_size < final_size + buffer_size)
                            new_size *= 2;
                        uint8_t* new_ptr = av_realloc(final_data, new_size);
                        if (!new_ptr) {
                            ret = -13;
                            goto cleanup;
                        }
                        final_data = new_ptr;
                        final_allocated = new_size;
                    }

                    memcpy(final_data + final_size, output[0], buffer_size);
                    final_size += buffer_size;
                }
            }
        }
        av_packet_unref(packet);
    }

    *out_data = final_data;
    *out_size = final_size;

    final_data = NULL; // prevent free in cleanup

cleanup:
    if (final_data)
        av_freep(&final_data);
    if (output) {
        av_freep(&output[0]);
        av_freep(&output);
    }
    if (swr_ctx)
        swr_free(&swr_ctx);
    if (frame)
        av_frame_free(&frame);
    if (packet)
        av_packet_free(&packet);
    if (codec_ctx)
        avcodec_free_context(&codec_ctx);
    if (avio_ctx_buffer)
        av_freep(&avio_ctx_buffer);
    if (avio_ctx)
        avio_context_free(&avio_ctx);
    if (fmt_ctx)
        avformat_close_input(&fmt_ctx);

    return ret;
}

#ifdef __cplusplus
}
#endif