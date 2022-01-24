#include "Decoder.h"

AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;

AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts)
{
    const AVPixelFormat* p;

    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

av::Decoder::Decoder
(
    AVFormatContext* fmt_ctx, int stream_index,
    Queue<Frame>* frame_q,
    std::function<void(const std::string&, MsgPriority, const std::string&)> fnMsg,
    AVHWDeviceType type
) :
    frame_q(frame_q)
{
    ex.fnMsgOut = fnMsg;

    try {



        stream = fmt_ctx->streams[stream_index];
        dec = avcodec_find_decoder(stream->codecpar->codec_id);

        ex.msg(streamInfo());
        
        if (!dec) 
            throw Exception(std::string("avcodec_find_decoder could not find ") + avcodec_get_name(stream->codecpar->codec_id));

        ex.ck(dec_ctx = avcodec_alloc_context3(dec), CmdTag::AAC3);
        ex.ck(avcodec_parameters_to_context(dec_ctx, stream->codecpar), CmdTag::APTC);

        ex.ck(frame = av_frame_alloc(), CmdTag::AFA);
        if (type != AV_HWDEVICE_TYPE_NONE) {
            ex.ck(sw_frame = av_frame_alloc(), CmdTag::AFA);
            for (int i = 0;; i++) {
                const AVCodecHWConfig* config;
                config = avcodec_get_hw_config(dec, i);

                if (config) {
                    std::stringstream str;
                    str << "device type: " << av_hwdevice_get_type_name(type);
                    ex.msg(str.str());
                }
                else {
                    throw Exception(std::string("Decoder ") + dec->name + std::string(" does not support device type ") + av_hwdevice_get_type_name(type));
                }

                if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
                    hw_pix_fmt = config->pix_fmt;
                    break;
                }
            }
            ex.ck(av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0), CmdTag::AHCC);
            dec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
            dec_ctx->get_format = get_hw_format;
            const char* hw_pix_fmt_name;
            hw_pix_fmt_name = av_get_pix_fmt_name(hw_pix_fmt);
            ex.msg(hw_pix_fmt_name, MsgPriority::INFO, "using hw pix fmt: ");

            ex.ck(sws_ctx = sws_getContext(dec_ctx->width, dec_ctx->height, AV_PIX_FMT_NV12,
                dec_ctx->width, dec_ctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL), CmdTag::SGC);

            cvt_frame = av_frame_alloc();
            cvt_frame->width = dec_ctx->width;
            cvt_frame->height = dec_ctx->height;
            cvt_frame->format = AV_PIX_FMT_YUV420P;
            av_frame_get_buffer(cvt_frame, 0);
        }

        ex.ck(avcodec_open2(dec_ctx, dec, NULL), CmdTag::AO2);

    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "Decoder constructor exception: ");
        close();
    }
}

av::Decoder::~Decoder()
{
    close();
}

void av::Decoder::close()
{
    av_frame_free(&frame);
    av_frame_free(&sw_frame);
    av_frame_free(&cvt_frame);
    avcodec_free_context(&dec_ctx);
    av_buffer_unref(&hw_device_ctx);
}

int av::Decoder::decode(AVPacket* pkt)
{
    int ret = 0;

    try {
        if (!pkt) ex.msg("decoder received NULL pkt eof", MsgPriority::INFO);

        if (!dec_ctx) throw Exception("dec_ctx null");

        ex.ck(ret = avcodec_send_packet(dec_ctx, pkt), CmdTag::ASP);

        while (ret >= 0) {
            ret = avcodec_receive_frame(dec_ctx, frame);
            if (ret < 0) {
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                    return 0;
                }
                else if (ret < 0) {
                    ex.ck(ret, "error during decoding");
                }
            }

            Frame f;
            if (frame->format == hw_pix_fmt) {
                ex.ck(ret = av_hwframe_transfer_data(sw_frame, frame, 0), CmdTag::AHTD);
                ex.ck(av_frame_copy_props(sw_frame, frame));
                ex.ck(sws_scale(sws_ctx, sw_frame->data, sw_frame->linesize, 0, dec_ctx->height, 
                    cvt_frame->data, cvt_frame->linesize), CmdTag::SS);
                cvt_frame->pts = sw_frame->pts;
                f = Frame(cvt_frame);
            }
            else {
                f = Frame(frame);
            }

            int size = dec_ctx->gop_size;
            f.m_frame->display_picture_number = dec_ctx->frame_number;
            f.set_rts(stream);
            frame_q->push(f);
        }
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "Decoder::decode exception: ");
        ret = -1;
    }

    return ret;
}

std::string av::Decoder::streamInfo()
{
    std::stringstream str;
    if (stream) {
        str << "codec name: " << avcodec_get_name(stream->codecpar->codec_id);
    }
    return str.str();
}

std::string av::Decoder::hardwareInfo()
{
}