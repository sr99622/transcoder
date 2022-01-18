#pragma once

#include "FileReader.h"
#include "Decoder.h"
#include "Encoder.h"
#include "Filter.h"
#include "CircularQueue.h"
#include "FileWriter.h"
#include "AudioGenerator.h"
#include "VideoGenerator.h"
#include "CircularQueue.h"

#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <mutex>
#include <sstream>

#include "Frame.h"
#include "EventLoop.h"
#include "Exception.h"

std::mutex m_mutex;
av::ExceptionHandler ex;

namespace av
{

class Factory
{
public:

    static void msgOut(const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (msg.length() > 0)
            std::cout << msg << std::endl;
    }

    static void msg(const std::string& msg, MsgPriority priority = MsgPriority::INFO, const std::string& qualifier = "")
    {
        std::stringstream str;
        switch (priority) {
        case INFO:
            str << "I: ";
            break;
        case CRITICAL:
            str << "CRITICAL ERROR! ";
            break;
        case DEBUG:
            str << "D: ";
            break;
        }
        str << qualifier << msg;
        msgOut(str.str());
    }

    static void mux(CircularQueue<Frame>* video_q, CircularQueue<Frame>* audio_q, CircularQueue<Frame>* interleaved_q)
    {
        bool capturingVideo = true;
        bool capturingAudio = true;
        Frame frmAudio;
        Frame frmVideo;

        bool muxing = true;

        while (muxing) {

            SDL_Delay(10);

            if (capturingAudio && audio_q->size() > 0) {
                frmAudio = audio_q->pop();
            }
            if (capturingVideo && video_q->size() > 0) {
                frmVideo = video_q->pop();
            }

            int comparator = 1;
            if (frmAudio.isValid() && frmVideo.isValid()) {
                if (frmVideo.m_rts > frmAudio.m_rts)
                    comparator = 1;
                else
                    comparator = -1;
            }
            else {
                if (frmVideo.isValid())
                    comparator = -1;
            }

            if (comparator > 0) {
                if (frmAudio.isValid()) {
                    if (frmAudio.m_frame->pts != AV_NOPTS_VALUE)
                        interleaved_q->push(frmAudio);
                    frmAudio.invalidate();
                    capturingVideo = false;
                    capturingAudio = true;
                }
                else {
                    capturingAudio = false;
                }
            }
            else {
                if (frmVideo.isValid()) {
                    if (frmAudio.m_frame->pts != AV_NOPTS_VALUE)
                        interleaved_q->push(frmVideo);
                    frmVideo.invalidate();
                    capturingVideo = true;
                    capturingAudio = false;
                }
                else {
                    capturingVideo = false;
                }
            }

            if (!(capturingVideo || capturingAudio)) {
                muxing = false;
            }
        }

        msgOut("mux loop end");
        interleaved_q->push(Frame(nullptr));
    }

    static void encode(Encoder* encoder, CircularQueue<Frame>* frame_q)
    {
        try {
            Frame frame;
            while (true)
            {
                frame_q->pop(frame);
                encoder->encode(frame);
                if (!frame.isValid())
                    break;
            }
            encoder->pkt_q->push(NULL);
        }
        catch (const QueueClosedException& e) {}
        msg("encode loop end");
    }

    static void pkt_drain(CircularQueue<AVPacket*>* pkt_q)
    {
        try {
            while (true)
            {
                AVPacket* pkt = pkt_q->pop();
                if (!pkt) {
                    break;
                }
                else {
                    std::cout << "pkt pts: " << pkt->pts << std::endl;
                    av_packet_free(&pkt);
                }
            }

        }
        catch (const QueueClosedException& e) {}
        msg("pkt drain loop end");
    }

    static void drain(CircularQueue<Frame>* frame_q)
    {
        try {
            Frame frame;
            while (true)
            {
                frame_q->pop(frame);
                if (!frame.isValid())
                    break;
            }
        }
        catch (const QueueClosedException& e) {}
        msg("drain loop end");
    }

    static void write(FileWriter* writer, CircularQueue<AVPacket*>* pkt_q)
    {
        try {
            while (AVPacket* pkt = pkt_q->pop())
            {
                writer->write(pkt);
                av_packet_free(&pkt);
            }
            writer->write(NULL);
        }
        catch (const QueueClosedException& e) {}
        msg("write loop end");
    }

    static void read(FileReader* reader, CircularQueue<AVPacket*>* video_pkt_q, CircularQueue<AVPacket*>* audio_pkt_q)
    {
        try {
            while (AVPacket* pkt = reader->read())
            {
                if (pkt->stream_index == reader->video_stream_index) {
                    if (video_pkt_q) video_pkt_q->push(pkt);
                }
                else if (pkt->stream_index == reader->audio_stream_index) {
                    if (audio_pkt_q) audio_pkt_q->push(pkt);
                }
            }

            if (video_pkt_q) video_pkt_q->push(NULL);
            if (audio_pkt_q) audio_pkt_q->push(NULL);
        }
        catch (const QueueClosedException& e) {}
        msg("read loop end");
    }

    static void decode(Decoder* decoder, CircularQueue<AVPacket*>* pkt_q)
    {
        try {
            while (AVPacket* pkt = pkt_q->pop())
            {
                ex.ck(decoder->decode(pkt));
                av_packet_free(&pkt);
            }
            decoder->decode(NULL);
            decoder->frame_q->push(Frame(nullptr));
        }
        catch (const QueueClosedException& e) {}
        catch (const Exception& e) {
            msg(e.what(), MsgPriority::CRITICAL, "decoder failed: ");
        }
        msg("decode loop end");
    }

    static void filter(Filter* filter, CircularQueue<Frame>* frame_q)
    {
        try {
            Frame frame;
            while (true)
            {
                frame_q->pop(frame);
                filter->filter(frame);
                if (!frame.isValid())
                    break;
            }
            filter->frame_out_q->push(Frame(nullptr));
        }
        catch (const QueueClosedException& e) {}
        msg("filter loop end");
    }
};

}

