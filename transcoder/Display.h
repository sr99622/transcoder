#pragma once

#include <SDL.h>
#include <chrono>
#include "Exception.h"
#include "Queue.h"
#include "Frame.h"
#include "EventLoop.h"
#include "Clock.h"
#include "Decoder.h"

namespace av
{

class Display
{
public:
    Display() {}
    Display
    (
        const Decoder& videoDecoder, const Decoder& audioDecoder, 
        std::function<void(const std::string&, MsgPriority, const std::string&)> fnMsg = nullptr,
        Queue<Frame>* video_in_q = nullptr, Queue<Frame>* audio_in_q = nullptr,
        Queue<Frame>* video_out_q = nullptr, Queue<Frame>* audio_out_q = nullptr
    );

    ~Display();

    void init(const Decoder& videoDecoder, const Decoder& audioDecoder);
    int initAudio(int sample_rate, AVSampleFormat sample_fmt, int channels, uint64_t channel_layout, int frame_size);
    int initVideo(int width, int height, AVPixelFormat pix_fmt);
    static void AudioCallback(void* userdata, uint8_t* stream, int len);
    void handleEvent(const SDL_Event& e, Frame& f);
    bool display();
    void close();

    bool paused = false;
    bool isPaused();
    void togglePause();

    std::string audioDeviceStatus();

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* texture = NULL;
    SDL_AudioSpec want = { 0 };
    SDL_AudioSpec have = { 0 };

    Queue<Frame>* video_in_q = nullptr;
    Queue<Frame>* video_out_q = nullptr;
    Queue<Frame>* audio_in_q = nullptr;
    Queue<Frame>* audio_out_q = nullptr;

    av::EventLoop eventLoop;

    SwrContext* swr_ctx;
    SDL_AudioDeviceID audioDeviceID;
    av::Clock rtClock;

    uint64_t channels() { return tmp.m_frame->channels; }
    int nb_samples() { return tmp.m_frame->nb_samples; }
    const uint8_t** data() { return (const uint8_t**)&tmp.m_frame->data[0]; }
    av::Frame tmp;

    uint8_t* audioBuf = nullptr;
    unsigned int audioBufSize = 0;
    unsigned int audioBufIndex = 0;
    int padding = 0;
    int dataSize = 0;

    uint64_t start_time;
    uint64_t duration;

    ExceptionHandler ex;
};

}


