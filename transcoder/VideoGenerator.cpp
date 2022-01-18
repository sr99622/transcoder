#include "VideoGenerator.h"
#include "Frame.h"

av::VideoGenerator::VideoGenerator(const StreamParameters& params)
{
    this->params = params;
    next_pts = 0;
    width = params.width;
    height = params.height;
    pix_fmt = params.pix_fmt;
    time_base = params.video_time_base;

    try {
        ex.ck(frame = av_frame_alloc(), CmdTag::AFA);
        frame->format = params.pix_fmt;
        frame->width = params.width;
        frame->height = params.height;
        ex.ck(av_frame_get_buffer(frame, 0), CmdTag::AFGB);
        //av.ck(av_frame_make_writable(frame), CmdTag::AFMW);

        ex.ck(yuv_frame = av_frame_alloc(), CmdTag::AFA);
        yuv_frame->format = AV_PIX_FMT_YUV420P;
        yuv_frame->width = params.width;
        yuv_frame->height = params.height;
        ex.ck(av_frame_get_buffer(yuv_frame, 0), CmdTag::AFGB);
        //av.ck(av_frame_make_writable(yuv_frame), CmdTag::AFMW);

    }
    catch (const Exception& e) {
        std::cerr << "VideoGenerator constructor exception: " << e.what() << std::endl;
    }
}

av::VideoGenerator::~VideoGenerator()
{
    av_frame_free(&frame);
    av_frame_free(&yuv_frame);
    sws_freeContext(sws_ctx);
}

void av::VideoGenerator::fillFrame(AVFrame* pict, int frame_index)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

AVFrame* av::VideoGenerator::getFrame()
{
    if (av_compare_ts(next_pts, time_base, STREAM_DURATION, av_make_q(1, 1)) > 0) {
        return NULL;
    }

    try {
        if (pix_fmt != AV_PIX_FMT_YUV420P) {
            if (sws_ctx == NULL) {
                ex.ck(sws_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height,
                    pix_fmt, SWS_BICUBIC, NULL, NULL, NULL), CmdTag::SGC);
                fillFrame(yuv_frame, next_pts);
                sws_scale(sws_ctx, (const uint8_t* const*)yuv_frame->data,
                    yuv_frame->linesize, 0, height, frame->data, frame->linesize);
            }
        }
        else {
            fillFrame(frame, next_pts);
        }
    }
    catch (const Exception& e) {
        std::cerr << "VideoGenerator::getFrame exception: " << e.what() << std::endl;
        return NULL;
    }

    frame->pts = next_pts++;
    return frame;
}
