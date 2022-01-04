#pragma once

#include "FileWriter.h"
#include "StreamParameters.h"

class AudioGenerator
{
public:
    AudioGenerator(const StreamParameters& params);
    ~AudioGenerator();
    AVFrame* getFrame();

    float t, tincr, tincr2;
    int64_t next_pts;
    int sample_rate;
    AVRational time_base;
    int samples_count = 0;

    AVFrame* frame;
    AVFrame* tmp;
    struct SwrContext* swr_ctx;
    int channels;
    AVExceptionHandler av;

};

