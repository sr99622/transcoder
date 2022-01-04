#pragma once

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include "../Utilities/avexception.h"
#include "StreamParameters.h"
#include "CircularQueue.h"
#include "Frame.h"

class Encoder
{
public:
    Encoder(void* parent, const StreamParameters& params, CircularQueue<AVPacket*>* pkt_q, AVMediaType media_type);
    ~Encoder();
    void openVideoStream(void* parent, const StreamParameters& params, CircularQueue<AVPacket*>* pkt_q);
    void openAudioStream(void* parent, const StreamParameters& params, CircularQueue<AVPacket*>* pkt_q);
    int encodeFrame(const Frame& frame);
    int encode(AVFrame* frame);
    void close();

    void* parent;
    AVMediaType media_type;
    AVStream* stream;
    AVCodecContext* enc_ctx;
    AVPacket* pkt;
    AVExceptionHandler av;

    AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE;
    AVBufferRef* hw_frames_ref = NULL;
    AVBufferRef* hw_device_ctx = NULL;
    AVFrame* hw_frame = NULL;

    CircularQueue<AVPacket*>* pkt_q;

};


