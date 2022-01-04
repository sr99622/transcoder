#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}

#include <iostream>
#include <string>

#define STREAM_DURATION   10.0


class StreamParameters
{
public:
    StreamParameters();
    StreamParameters(AVCodecContext* ctx);
    ~StreamParameters();
    void readAudioCtx(AVCodecContext* ctx);
    void readVideoCtx(AVCodecContext* ctx);
    void showAvailableOutputFormats();
    void showEncoderProperties(const char* codec_name);
    std::string toString(AVMediaType media_type) const;

    // FORMAT
    const char* format = NULL;

    // VIDEO
    AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
    int width = 0;
    int height = 0;
    int video_bit_rate = 0;
    int frame_rate = 0;
    int gop_size = 0;
    AVRational video_time_base = av_make_q(0, 0);

    // AUDIO
    AVSampleFormat sample_fmt = AV_SAMPLE_FMT_NONE;
    uint64_t channel_layout = 0;
    int audio_bit_rate = 0;
    int sample_rate = 0;
    int nb_samples = 0;
    int channels = 0;
    AVRational audio_time_base = av_make_q(0, 0);

    // CODEC
    const char* video_codec_name = NULL;
    const char* audio_codec_name = NULL;
    const char* profile = NULL;

    // HARDWARE
    AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE;
    AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
    AVPixelFormat sw_pix_fmt = AV_PIX_FMT_NONE;

    AVDictionary* opts = NULL;
};

