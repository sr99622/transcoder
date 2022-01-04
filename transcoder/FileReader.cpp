#include "FileReader.h"

FileReader::FileReader(const char* filename)
{
    try {
        av.ck(avformat_open_input(&fmt_ctx, filename, NULL, NULL), CmdTag::AOI);
        av.ck(avformat_find_stream_info(fmt_ctx, NULL), CmdTag::AFSI);

        video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if (video_stream_index < 0)
            std::cout << "av_find_best_stream could not find video stream" << std::endl;

        audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        if (audio_stream_index < 0)
            std::cout << "av_find_best_stream could not find audio stream" << std::endl;
    }
    catch (const AVException& e) {
        std::cout << "FileReader constructor exception: " << e.what() << std::endl;
    }
}

FileReader::~FileReader()
{
    avformat_close_input(&fmt_ctx);
}

AVPacket* FileReader::read_packet()
{
    int ret = 0;
    AVPacket* pkt = av_packet_alloc();

    try {
        av.ck(ret = av_read_frame(fmt_ctx, pkt), CmdTag::ARF);
    }
    catch (const AVException& e) {
        if (ret != AVERROR_EOF)
            std::cout << "FileReader::read_packet exception: " << e.what() << std::endl;
        else
            std::cout << "FileReader EOF" << std::endl;

        av_packet_free(&pkt);
    }

    return pkt;
}
