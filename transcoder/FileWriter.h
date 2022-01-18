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

#include <mutex>

#include "Exception.h"

#include "StreamParameters.h"

namespace av
{

class FileWriter
{
public:
    FileWriter(const StreamParameters& params);
    ~FileWriter();
    void open(const char* filename);
    void write(AVPacket* pkt);
    void close();

    AVFormatContext* fmt_ctx;
    int video_stream_id = AVERROR_STREAM_NOT_FOUND;
    int audio_stream_id = AVERROR_STREAM_NOT_FOUND;
    AVDictionary* opts;
    ExceptionHandler ex;

    std::mutex mutex;

};

}


