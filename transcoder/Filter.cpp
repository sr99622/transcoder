#include "Filter.h"

av::Filter::Filter(const Decoder& decoder, const char* description, Queue<Frame>* frame_out_q) : frame_out_q(frame_out_q)
{
    char args[512];
    int ret = 0;
    const AVFilter* buffersrc = avfilter_get_by_name("buffer");
    const AVFilter* buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();
    stream = decoder.stream;
    time_base = decoder.stream->time_base;
    //enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    std::cout << "time base: " << time_base.num << " / " << time_base.den << std::endl;
    std::cout << "avg_frame_rate: " << stream->avg_frame_rate.num << " / " << stream->avg_frame_rate.den << std::endl;

    snprintf(args, sizeof(args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        decoder.dec_ctx->width, decoder.dec_ctx->height, decoder.dec_ctx->pix_fmt,
        time_base.num, time_base.den,
        decoder.dec_ctx->sample_aspect_ratio.num, decoder.dec_ctx->sample_aspect_ratio.den);

    try {
        ex.ck(frame = av_frame_alloc(), CmdTag::AFA);
        ex.ck(graph = avfilter_graph_alloc(), CmdTag::AGA);
        ex.ck(avfilter_graph_create_filter(&src_ctx, buffersrc, "in", args, NULL, graph), CmdTag::AGCF);
        ex.ck(avfilter_graph_create_filter(&sink_ctx, buffersink, "out", NULL, NULL, graph), CmdTag::AGCF);
        //ex.ck(av_opt_set_int_list(sink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN), CmdTag::AOSIL);

        outputs->name = av_strdup("in");
        outputs->filter_ctx = src_ctx;
        outputs->pad_idx = 0;
        outputs->next = NULL;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = sink_ctx;
        inputs->pad_idx = 0;
        inputs->next = NULL;

        ex.ck(avfilter_graph_parse_ptr(graph, description, &inputs, &outputs, NULL), CmdTag::AGPP);
        ex.ck(avfilter_graph_config(graph, NULL), CmdTag::AGC);

        for (int i = 0; i < graph->nb_filters; i++) {
            AVFilterContext* filter = graph->filters[i];
            std::stringstream str;
            str << "name: " << filter->name << " type: " << filter->filter->name;
            ex.msg(str.str());

            if (!strcmp(filter->filter->name, "fps")) {
                ex.msg("fps conversion");
                std::size_t start = std::string(description).find("fps=");
                std::size_t end = std::string(description).find(",", start);
                std::string fps_arg = std::string(description).substr(start + 4, end);
                ex.msg(fps_arg + "  fps_arg: ");
                int fps = std::stoi(fps_arg);
                std::cout << "FPS: " << fps << std::endl;
                double factor = av_q2d(stream->time_base)* av_q2d(stream->avg_frame_rate);
                int time_base_den = fps / factor;
                std::cout << "factor: " << time_base_den << std::endl;
                time_base = av_make_q(1, fps);
                //stream->avg_frame_rate = av_make_q(fps, 1);
            }

            if (!strcmp(filter->filter->name, "format")) {
                ex.msg("format conversion");
                std::size_t start = std::string(description).find("format=");
                std::size_t end = std::string(description).find(",", start);
                std::string format_arg = std::string(description).substr(start + 7, end);
                ex.msg(format_arg + "  fps_arg: ");
            }

        }

    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "Filter constructor exception: ");
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
}

av::Filter::~Filter()
{
    avfilter_graph_free(&graph);
    av_frame_free(&frame);
}

int av::Filter::filter(const Frame& f)
{
    int ret = 0;

    try {
        ex.ck(av_buffersrc_add_frame_flags(src_ctx, f.m_frame, AV_BUFFERSRC_FLAG_KEEP_REF), CmdTag::ABAFF);
        while (true) {
            ret = av_buffersink_get_frame(sink_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
                throw Exception("av_buffersink_get_frame");

            tmp = Frame(frame);
            //tmp.set_rts(stream);

            double factor = 1000 * av_q2d(time_base);
            tmp.m_rts = (tmp.pts()) * factor;

            frame_out_q->push(tmp);
            av_frame_unref(frame);
        }
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "Filter::filter exception: ");
    }

    return ret;
}