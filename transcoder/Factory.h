#pragma once

#include <mutex>
#include <sstream>

#include "FileReader.h"
#include "Decoder.h"
#include "Encoder.h"
#include "Filter.h"
#include "Queue.h"
#include "FileWriter.h"
#include "Queue.h"
#include "Darknet.h"
#include "PyModelSSD.h"

#define SDL_MAIN_HANDLED

#include <SDL.h>

#include "Display.h"
#include "Frame.h"
#include "EventLoop.h"
#include "Exception.h"

std::mutex m_mutex;

namespace av
{

ExceptionHandler ex;

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

    static void mux(Queue<Frame>* video_q, Queue<Frame>* audio_q, Queue<Frame>* interleaved_q)
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

    static void encode(Encoder* encoder, Queue<Frame>* frame_q)
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

    static void pkt_drain(Queue<AVPacket*>* pkt_q)
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

    static void drain(Queue<Frame>* frame_q)
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

    static void write(FileWriter* writer, Queue<AVPacket*>* pkt_q)
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

    static void read(FileReader* reader, Queue<AVPacket*>* video_pkt_q, Queue<AVPacket*>* audio_pkt_q)
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

    static void decode(Decoder* decoder, Queue<AVPacket*>* pkt_q)
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

    static void filter(Filter* filter, Queue<Frame>* frame_q)
    {
        try {
            Frame f;
            while (true)
            {
                frame_q->pop(f);
                filter->filter(f);
                if (!f.isValid())
                    break;
            }
            filter->frame_out_q->push(Frame(nullptr));
        }
        catch (const QueueClosedException& e) {}
        msg("filter loop end");
    }

    static void detect(Darknet* detector, Queue<Frame>* frame_q)
    {
        try {
            Frame f;
            while (true)
            {
                frame_q->pop(f);
                if (f.isValid()) {
                    detector->detect(&f);
                    //std::cout << "size: " << f.detections.size() << std::endl;
                    for (int i = 0; i < f.detections.size(); i++)
                        f.drawBox(f.detections[i]);

                    detector->frame_out_q->push(f);
                }
                else {
                    detector->frame_out_q->push(Frame(nullptr));
                    break;
                }
            }
        }
        catch (const QueueClosedException& e) {}
        msg("detect loop end");
    }

    static int py_detect(Queue<Frame>* frame_out_q, Queue<Frame>* frame_in_q)
    {
        try {
            PyModelSSD model("ssd_mobilenet_v2_320x320_coco17_tpu-8/saved_model", frame_out_q, 50);
            if (!PyArray_API) import_array();
            if (PyErr_Occurred()) throw Exception("PyErr_Occurred");

            Frame f;
            while (true)
            {
                frame_in_q->pop(f);
                if (f.isValid()) {
                    std::vector<av::Box<int>> boxes;
                    
                    model.detect(f.mat(), &boxes);
                    for (int i = 0; i < boxes.size(); i++) {
                        f.detections.push_back(boxes[i].to_bbox());
                        f.drawBox(boxes[i].to_bbox());
                    }
                    
                    frame_out_q->push(f);
                }
                else {
                    frame_out_q->push(Frame(nullptr));
                }
            }
        }
        catch (const QueueClosedException& e) {}
        catch (const Exception& e) { msg(e.what(), MsgPriority::CRITICAL, "py_detect "); }
        msg("py_detect loop end");
    }
};

}

