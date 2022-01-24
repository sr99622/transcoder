#include "FileReader.h"
#include <iomanip>

av::FileReader::FileReader(const char* filename, std::function<void(const std::string&, MsgPriority, const std::string&)> fnMsg)
{
    try {
        ex.fnMsgOut = fnMsg;
        ex.ck(avformat_open_input(&fmt_ctx, filename, NULL, NULL), CmdTag::AOI);
        ex.ck(avformat_find_stream_info(fmt_ctx, NULL), CmdTag::AFSI);

        video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (video_stream_index < 0)
            ex.msg("av_find_best_stream could not find video stream", MsgPriority::INFO);

        audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        if (audio_stream_index < 0)
            ex.msg("av_find_best_stream could not find audio stream", MsgPriority::INFO);
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "FileReader constructor exception: ");
    }
}

av::FileReader::~FileReader()
{
    avformat_close_input(&fmt_ctx);
}

AVPacket* av::FileReader::read()
{
    if (closed)
        return NULL;

    int ret = 0;
    AVPacket* pkt = av_packet_alloc();

    try {
        ex.ck(ret = av_read_frame(fmt_ctx, pkt), CmdTag::ARF);
    }
    catch (const Exception& e) {
        if (ret == AVERROR_EOF)
            ex.msg("FileReader EOF", MsgPriority::INFO);
        else
            ex.msg(e.what(), MsgPriority::CRITICAL, "FileReader::read_packet exception: ");

        av_packet_free(&pkt);
        closed = true;
    }

    return pkt;
}



/*
if (pkt->stream_index == video_stream_index) {
    char buf[16];
    std::stringstream str;
    str << "  PKT HEADER: ";
    for (int i = 0; i < 12; i++) {
        sprintf(buf, "%02x", (int)pkt->data[i] & 0x1F);
        //sprintf(buf, "%02x", 0x67 & 0x1F);
        str << " " << buf;
    }
    str << " SIZE: " << pkt->size;
    ex.msg(str.str());
}
*/

