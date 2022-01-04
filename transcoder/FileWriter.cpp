#include "FileWriter.h"

FileWriter::FileWriter(const StreamParameters& params)
{
    opts = params.opts;

    try {
        //av.ck(avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, filename), CmdTag::AAOC2);
        av.ck(avformat_alloc_output_context2(&fmt_ctx, NULL, params.format, NULL), CmdTag::AAOC2);
    }
    catch (const AVException& e) {
        av.showError(std::string("FileWriter constructor exception: ") + e.what());
    }
}

FileWriter::~FileWriter()
{
    avformat_free_context(fmt_ctx);
}

void FileWriter::openFile(const char* filename)
{
    try {
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
            av.ck(avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE), CmdTag::AO);

        av.ck(avformat_write_header(fmt_ctx, &opts), CmdTag::AWH);
    }
    catch (const AVException*& e) {
        av.showError(std::string("FileWriter::openFile exception: ") + e->what());
    }
}

void FileWriter::writeFrame(AVPacket* pkt)
{
    std::unique_lock<std::mutex> lock(mutex);
    try {
        av.ck(av_interleaved_write_frame(fmt_ctx, pkt), CmdTag::AIWF);
    }
    catch (const AVException& e) {
        av.showError(std::string("FileWriter::writeFrame exception: ") + e.what());
    }
}

void FileWriter::closeFile()
{
    try {
        av.ck(av_write_trailer(fmt_ctx), CmdTag::AWT);

        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
            av.ck(avio_closep(&fmt_ctx->pb), CmdTag::AC);
    }
    catch (const AVException& e) {
        av.showError(std::string("FileWriter::close exception: ") + e.what());
    }
}


