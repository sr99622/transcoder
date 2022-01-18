#include "FileWriter.h"

av::FileWriter::FileWriter(const StreamParameters& params)
{
    opts = params.opts;

    try {
        //av.ck(avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, filename), CmdTag::AAOC2);
        ex.ck(avformat_alloc_output_context2(&fmt_ctx, NULL, params.format, NULL), CmdTag::AAOC2);
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "FileWriter constructor exception: ");
    }
}

av::FileWriter::~FileWriter()
{
    avformat_free_context(fmt_ctx);
}

void av::FileWriter::open(const char* filename)
{
    try {
        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
            ex.ck(avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE), CmdTag::AO);

        ex.ck(avformat_write_header(fmt_ctx, &opts), CmdTag::AWH);
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "FileWriter::openFile exception: ");
    }
}

void av::FileWriter::write(AVPacket* pkt)
{
    std::unique_lock<std::mutex> lock(mutex);
    try {
        ex.ck(av_interleaved_write_frame(fmt_ctx, pkt), CmdTag::AIWF);
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "FileWriter::writeFrame exception: ");
    }
}

void av::FileWriter::close()
{
    try {
        ex.ck(av_write_trailer(fmt_ctx), CmdTag::AWT);

        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
            ex.ck(avio_closep(&fmt_ctx->pb), CmdTag::AC);
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "FileWriter::close exception: ");
    }
}


