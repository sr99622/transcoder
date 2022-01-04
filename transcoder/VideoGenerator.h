#pragma once

#include "FileWriter.h"
#include "StreamParameters.h"

class VideoGenerator
{
public:
    VideoGenerator(const StreamParameters& params);
    ~VideoGenerator();
    AVFrame* getFrame();
    void fillFrame(AVFrame* pict, int frame_index);

    int width;
    int height;
    int64_t next_pts;
    SwsContext* sws_ctx;
    AVPixelFormat pix_fmt;
    AVRational time_base;
    AVFrame* frame;
    StreamParameters params;
    AVFrame* yuv_frame;
    AVExceptionHandler av;
};

