#include "Encoder.h"
#include "FileWriter.h"

Encoder::Encoder(void* parent, const StreamParameters& params, CircularQueue<AVPacket*>* pkt_q, AVMediaType media_type)
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
        av.showError("Encoder constructor failed unknown media type");
    }
}

Encoder::~Encoder()
{
    close();
}

void Encoder::close()
{
    avcodec_free_context(&enc_ctx);
    av_packet_free(&pkt);
    av_frame_free(&hw_frame);
    av_buffer_unref(&hw_device_ctx);
}

void Encoder::openVideoStream(void* parent, const StreamParameters& params, CircularQueue<AVPacket*>* pkt_q)
{
    this->parent = parent;
    this->pkt_q = pkt_q;
    FileWriter* writer = ((FileWriter*)parent);
    AVFormatContext* fmt_ctx = writer->fmt_ctx;
    AVCodecID codec_id = fmt_ctx->oformat->video_codec;
    hw_device_type = params.hw_device_type;

    try {
        AVCodec* codec;
        av.ck(pkt = av_packet_alloc(), CmdTag::APA);

        if (hw_device_type != AV_HWDEVICE_TYPE_NONE) {
            av.ck(codec = avcodec_find_encoder_by_name(params.video_codec_name), CmdTag::AFEBN);
        }
        else {
            av.ck(codec = avcodec_find_encoder(fmt_ctx->oformat->video_codec), CmdTag::AFE);
        }

        av.ck(stream = avformat_new_stream(fmt_ctx, NULL), CmdTag::ANS);
        stream->id = fmt_ctx->nb_streams - 1;
        writer->video_stream_id = stream->id;
        av.ck(enc_ctx = avcodec_alloc_context3(codec), CmdTag::AAC3);

        enc_ctx->codec_id = codec_id;
        enc_ctx->bit_rate = params.video_bit_rate;
        enc_ctx->width = params.width;
        enc_ctx->height = params.height;
        stream->time_base = av_make_q(1, params.frame_rate);
        enc_ctx->time_base = stream->time_base;
        enc_ctx->gop_size = params.gop_size;

        if (hw_device_type != AV_HWDEVICE_TYPE_NONE) {
            enc_ctx->pix_fmt = params.hw_pix_fmt;
            av_opt_set(enc_ctx->priv_data, "profile", params.profile, 0);

            av.ck(av_hwdevice_ctx_create(&hw_device_ctx, hw_device_type, NULL, NULL, 0), CmdTag::AHCC);
            av.ck(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx), CmdTag::AHCA);

            AVHWFramesContext* frames_ctx = (AVHWFramesContext*)(hw_frames_ref->data);
            frames_ctx->format = params.hw_pix_fmt;
            frames_ctx->sw_format = params.sw_pix_fmt;
            frames_ctx->width = params.width;
            frames_ctx->height = params.height;
            frames_ctx->initial_pool_size = 20;

            av.ck(av_hwframe_ctx_init(hw_frames_ref), CmdTag::AHCI);
            av.ck(enc_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref), CmdTag::ABR);
            av_buffer_unref(&hw_frames_ref);

            av.ck(hw_frame = av_frame_alloc(), CmdTag::AFA);
            av.ck(av_hwframe_get_buffer(enc_ctx->hw_frames_ctx, hw_frame, 0), CmdTag::AHGB);
        }
        else {
            enc_ctx->pix_fmt = params.pix_fmt;
        }

        if (enc_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) enc_ctx->max_b_frames = 2;
        if (enc_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) enc_ctx->mb_decision = 2;

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        AVDictionary* opts = params.opts;
        av.ck(avcodec_open2(enc_ctx, codec, &opts), CmdTag::AO2);
        av.ck(avcodec_parameters_from_context(stream->codecpar, enc_ctx), CmdTag::APFC);
    }
    catch (const AVException& e) {
        std::cerr << "VideoStream constructor exception: " << e.what() << std::endl;
        close();
    }
}

void Encoder::openAudioStream(void* parent, const StreamParameters& params, CircularQueue<AVPacket*>* pkt_q)
{
    this->parent = parent;
    this->pkt_q = pkt_q;
    FileWriter* writer = ((FileWriter*)parent);
    AVFormatContext* fmt_ctx = writer->fmt_ctx;
    AVCodecID codec_id = fmt_ctx->oformat->audio_codec;

    try {
        AVCodec* codec;
        av.ck(codec = avcodec_find_encoder(codec_id), std::string("avcodec_find_decoder could not find ") + avcodec_get_name(codec_id));
        av.ck(pkt = av_packet_alloc(), CmdTag::APA);
        av.ck(stream = avformat_new_stream(fmt_ctx, NULL), CmdTag::ANS);
        stream->id = fmt_ctx->nb_streams - 1;
        writer->audio_stream_id = stream->id;
        av.ck(enc_ctx = avcodec_alloc_context3(codec), CmdTag::AAC3);

        enc_ctx->sample_fmt = params.sample_fmt;
        enc_ctx->bit_rate = params.audio_bit_rate;
        enc_ctx->sample_rate = params.sample_rate;
        enc_ctx->channel_layout = params.channel_layout;
        enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
        stream->time_base = av_make_q(1, enc_ctx->sample_rate);

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        AVDictionary* opts = params.opts;
        av.ck(avcodec_open2(enc_ctx, codec, &opts), CmdTag::AO2);
        av.ck(avcodec_parameters_from_context(stream->codecpar, enc_ctx), CmdTag::APFC);

        //std::cout << "AudioStream stream time_base: " << stream->time_base.num << " / " << stream->time_base.den << std::endl;
        //std::cout << "AudioStream enc time_base: " << enc_ctx->time_base.num << " / " << enc_ctx->time_base.den << std::endl;

    }
    catch (const AVException& e) {
        av.showError(std::string("AudioStream constructor exception: ") + e.what());
        close();
    }
}

int Encoder::encodeFrame(const Frame& frame)
{
    return encode(frame.frame);
}

int Encoder::encode(AVFrame* frame)
{
    int ret = 0;
    try {

        if (!frame) std::cout << "recieved NULL frame eof" << std::endl;

        if (hw_device_type != AV_HWDEVICE_TYPE_NONE) {
            if (frame) {
                av_frame_copy_props(hw_frame, frame);
                av.ck(av_hwframe_transfer_data(hw_frame, frame, 0), CmdTag::AHTD);
            }
            av.ck(avcodec_send_frame(enc_ctx, hw_frame), CmdTag::ASF);
        }
        else {
            av.ck(avcodec_send_frame(enc_ctx, frame), CmdTag::ASF);
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(enc_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                av.ck(ret, CmdTag::ARP);
            }

            //av_packet_rescale_ts(pkt, enc_ctx->time_base, stream->time_base);
            pkt->stream_index = stream->index;

            //std::cout << "video out pkt pts: " << pkt->pts << std::endl;
            AVPacket* tmp = av_packet_alloc();
            tmp = av_packet_clone(pkt);
            pkt_q->push(tmp);
        }
    }
    catch (const AVException& e) {
        av.showError(std::string("Encode::encode exception: ") + e.what());
    }

    return ret;
}

