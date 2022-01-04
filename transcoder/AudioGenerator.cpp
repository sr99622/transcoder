#include "AudioGenerator.h"

AudioGenerator::AudioGenerator(const StreamParameters& params)
{
    time_base = params.audio_time_base;
    channels = params.channels;
    sample_rate = params.sample_rate;

    t = 0;
    next_pts = 0;
    tincr = 2 * M_PI * 110.0 / params.sample_rate;
    tincr2 = 2 * M_PI * 110.0 / params.sample_rate / params.sample_rate;

    try {
        av.ck(frame = av_frame_alloc(), CmdTag::AFA);
        frame->format = params.sample_fmt;
        frame->channel_layout = params.channel_layout;
        frame->sample_rate = params.sample_rate;
        frame->nb_samples = params.nb_samples;
        av.ck(av_frame_get_buffer(frame, 0), CmdTag::AFGB);
        av.ck(av_frame_make_writable(frame), CmdTag::AFMW);

        av.ck(tmp = av_frame_alloc(), CmdTag::AFA);
        tmp->format = params.sample_fmt;
        tmp->channel_layout = params.channel_layout;
        tmp->sample_rate = params.sample_rate;
        tmp->nb_samples = params.nb_samples;
        av.ck(av_frame_get_buffer(tmp, 0), CmdTag::AFGB);
        av.ck(av_frame_make_writable(tmp), CmdTag::AFMW);

        av.ck(swr_ctx = swr_alloc(), CmdTag::SA);
        av_opt_set_int(swr_ctx, "in_channel_count", params.channels, 0);
        av_opt_set_int(swr_ctx, "in_sample_rate", params.sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        //av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", params.sample_fmt, 0);
        av_opt_set_int(swr_ctx, "out_channel_count", params.channels, 0);
        av_opt_set_int(swr_ctx, "out_sample_rate", params.sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", params.sample_fmt, 0);
        av.ck(swr_init(swr_ctx), CmdTag::SI);

    }
    catch (const std::exception& e) {
        std::cerr << "AudioStream::allocateFrame exception: " << e.what() << std::endl;
    }
}

AudioGenerator::~AudioGenerator()
{
    av_frame_free(&frame);
    av_frame_free(&tmp);
}

AVFrame* AudioGenerator::getFrame()
{
    int j, i, v;
    int16_t* q = (int16_t*)frame->data[0];

    // check if we want to generate more frames 
    if (av_compare_ts(next_pts, time_base, STREAM_DURATION, av_make_q(1, 1)) > 0) {
        return NULL;
    }

    for (j = 0; j < frame->nb_samples; j++) {
        v = (int)(sin(t) * 10000);
        for (i = 0; i < channels; i++)
            *q++ = v;
        t += tincr;
        tincr += tincr2;
    }

    frame->pts = next_pts;
    next_pts += frame->nb_samples;

    av.ck(swr_convert(swr_ctx, tmp->data, tmp->nb_samples,
        (const uint8_t**)frame->data, frame->nb_samples), CmdTag::SC);

    tmp->pts = av_rescale_q(samples_count, av_make_q(1, sample_rate), time_base);
    samples_count += tmp->nb_samples;

    return tmp;
}
