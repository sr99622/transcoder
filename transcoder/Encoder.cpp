#include "Encoder.h"
#include "FileWriter.h"

av::Encoder::Encoder(void* parent, const StreamParameters& params, Queue<AVPacket*>* pkt_q, AVMediaType media_type)
{
    this->media_type = media_type;
    switch (media_type) {
    case AVMEDIA_TYPE_VIDEO:
        openVideoStream(parent, params, pkt_q);
        break;
    case AVMEDIA_TYPE_AUDIO:
        openAudioStream(parent, params, pkt_q);
        break;
    default:
        ex.msg("Encoder constructor failed unknown media type", MsgPriority::CRITICAL);
    }
}

av::Encoder::~Encoder()
{
    close();
}

void av::Encoder::close()
{
    if (enc_ctx)       avcodec_free_context(&enc_ctx);
    if (pkt)           av_packet_free(&pkt);
    if (hw_frame)      av_frame_free(&hw_frame);
    if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);
    if (cvt_frame)     av_frame_free(&cvt_frame);
}

void av::Encoder::openVideoStream(void* parent, const StreamParameters & params, Queue<AVPacket*>*pkt_q)
{
    this->parent = parent;
    this->pkt_q = pkt_q;
    FileWriter* writer = ((FileWriter*)parent);
    AVFormatContext* fmt_ctx = writer->fmt_ctx;
    AVCodecID codec_id = fmt_ctx->oformat->video_codec;
    hw_device_type = params.hw_device_type;

    try {
        AVCodec* codec;
        ex.ck(pkt = av_packet_alloc(), CmdTag::APA);

        if (hw_device_type != AV_HWDEVICE_TYPE_NONE) {
            ex.ck(codec = avcodec_find_encoder_by_name(params.video_codec_name), CmdTag::AFEBN);
        }
        else {
            ex.ck(codec = avcodec_find_encoder(fmt_ctx->oformat->video_codec), CmdTag::AFE);
        }

        ex.ck(stream = avformat_new_stream(fmt_ctx, NULL), CmdTag::ANS);
        stream->id = fmt_ctx->nb_streams - 1;
        writer->video_stream_id = stream->id;
        ex.ck(enc_ctx = avcodec_alloc_context3(codec), CmdTag::AAC3);

        enc_ctx->codec_id = codec_id;
        enc_ctx->bit_rate = params.video_bit_rate;
        enc_ctx->width = params.width;
        enc_ctx->height = params.height;
        stream->time_base = av_make_q(1, params.frame_rate);
        enc_ctx->time_base = stream->time_base;
        enc_ctx->gop_size = params.gop_size;

        cvt_frame = av_frame_alloc();
        cvt_frame->width = enc_ctx->width;
        cvt_frame->height = enc_ctx->height;
        cvt_frame->format = AV_PIX_FMT_YUV420P;
        av_frame_get_buffer(cvt_frame, 0);
        av_frame_make_writable(cvt_frame);

        if (hw_device_type != AV_HWDEVICE_TYPE_NONE) {
            enc_ctx->pix_fmt = params.hw_pix_fmt;
            av_opt_set(enc_ctx->priv_data, "profile", params.profile, 0);

            ex.ck(av_hwdevice_ctx_create(&hw_device_ctx, hw_device_type, NULL, NULL, 0), CmdTag::AHCC);
            ex.ck(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx), CmdTag::AHCA);

            AVHWFramesContext* frames_ctx = (AVHWFramesContext*)(hw_frames_ref->data);
            frames_ctx->format = params.hw_pix_fmt;
            frames_ctx->sw_format = params.sw_pix_fmt;
            frames_ctx->width = params.width;
            frames_ctx->height = params.height;
            frames_ctx->initial_pool_size = 20;

            ex.ck(av_hwframe_ctx_init(hw_frames_ref), CmdTag::AHCI);
            ex.ck(enc_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref), CmdTag::ABR);
            av_buffer_unref(&hw_frames_ref);

            ex.ck(hw_frame = av_frame_alloc(), CmdTag::AFA);
            ex.ck(av_hwframe_get_buffer(enc_ctx->hw_frames_ctx, hw_frame, 0), CmdTag::AHGB);
        }
        else {
            enc_ctx->pix_fmt = params.pix_fmt;
        }

        if (enc_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) enc_ctx->max_b_frames = 2;
        if (enc_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) enc_ctx->mb_decision = 2;

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        AVDictionary* opts = params.opts;
        ex.ck(avcodec_open2(enc_ctx, codec, &opts), CmdTag::AO2);
        ex.ck(avcodec_parameters_from_context(stream->codecpar, enc_ctx), CmdTag::APFC);
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "VideoStream constructor exception: ");
        close();
    }
}

void av::Encoder::openAudioStream(void* parent, const StreamParameters& params, Queue<AVPacket*>* pkt_q)
{
    this->parent = parent;
    this->pkt_q = pkt_q;
    FileWriter* writer = ((FileWriter*)parent);
    AVFormatContext* fmt_ctx = writer->fmt_ctx;
    AVCodecID codec_id = fmt_ctx->oformat->audio_codec;

    try {
        AVCodec* codec;
        codec = avcodec_find_encoder(codec_id);

        if (codec)
            ex.msg(std::string("Encoder opened audio stream codec ") + codec->long_name);
        else
            throw Exception(std::string("avcodec_find_decoder could not find ") + avcodec_get_name(codec_id));

        ex.ck(pkt = av_packet_alloc(), CmdTag::APA);
        ex.ck(stream = avformat_new_stream(fmt_ctx, NULL), CmdTag::ANS);
        stream->id = fmt_ctx->nb_streams - 1;
        writer->audio_stream_id = stream->id;
        ex.ck(enc_ctx = avcodec_alloc_context3(codec), CmdTag::AAC3);

        enc_ctx->sample_fmt = params.sample_fmt;
        enc_ctx->bit_rate = params.audio_bit_rate;
        enc_ctx->sample_rate = params.sample_rate;
        enc_ctx->channel_layout = params.channel_layout;
        enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
        //enc_ctx->frame_size = params.nb_samples; // ???
        stream->time_base = av_make_q(1, enc_ctx->sample_rate);

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        AVDictionary* opts = params.opts;
        ex.ck(avcodec_open2(enc_ctx, codec, &opts), CmdTag::AO2);
        ex.ck(avcodec_parameters_from_context(stream->codecpar, enc_ctx), CmdTag::APFC);

        //std::cout << "AudioStream stream time_base: " << stream->time_base.num << " / " << stream->time_base.den << std::endl;
        //std::cout << "AudioStream enc time_base: " << enc_ctx->time_base.num << " / " << enc_ctx->time_base.den << std::endl;

    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "AudioStream constructor exception: ");
        close();
    }
}

int av::Encoder::encode(Frame& f)
{
    int ret = 0;
    try {

        f.set_pts(stream);

        if (!sws_ctx) {
            if (f.mediaType() == AVMEDIA_TYPE_VIDEO) {
                std::stringstream str;
                str << "Encoder input frame width: " << f.m_frame->width << " height: " << f.m_frame->height;
                const char* pix_fmt_name = av_get_pix_fmt_name((AVPixelFormat)f.m_frame->format);
                str << " format: " << (pix_fmt_name ? pix_fmt_name : "NULL");
                ex.msg(str.str(), MsgPriority::INFO);

                ex.ck(sws_ctx = sws_getContext(f.m_frame->width, f.m_frame->height, (AVPixelFormat)f.m_frame->format,
                    enc_ctx->width, enc_ctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL), CmdTag::SGC);

            }
        }

        AVFrame* frame = f.m_frame;

        if (frame) {

            if (f.mediaType() == AVMEDIA_TYPE_VIDEO && frame->format != AV_PIX_FMT_YUV420P) {
                ex.ck(sws_scale(sws_ctx, frame->data, frame->linesize, 0, enc_ctx->height,
                    cvt_frame->data, cvt_frame->linesize), CmdTag::SS);
                cvt_frame->pts = frame->pts;
                frame = cvt_frame;
            }
        }

        if (hw_device_type != AV_HWDEVICE_TYPE_NONE) {
            if (frame) {
                av_frame_copy_props(hw_frame, frame);
                ex.ck(av_hwframe_transfer_data(hw_frame, frame, 0), CmdTag::AHTD);
                ex.ck(avcodec_send_frame(enc_ctx, hw_frame), CmdTag::ASF);
            }
            else {
                ex.ck(avcodec_send_frame(enc_ctx, NULL), CmdTag::ASF);
            }
        }
        else {
            ex.ck(avcodec_send_frame(enc_ctx, frame), CmdTag::ASF);
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                ex.ck(ret, CmdTag::ARP);
            }

            pkt->stream_index = stream->index;

            AVPacket* tmp = av_packet_alloc();

            tmp = av_packet_clone(pkt);
            pkt_q->push(tmp);
        }
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "Encode::encode exception: ");
    }

    return ret;
}

