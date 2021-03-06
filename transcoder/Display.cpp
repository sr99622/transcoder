#include "Display.h"

av::Display::Display
(
    const Decoder& videoDecoder, const Decoder& audioDecoder,
    std::function<void(const std::string&, MsgPriority, const std::string&)> fnMsg,
    Queue<Frame>* video_in_q, Queue<Frame>* audio_in_q,
    Queue<Frame>* video_out_q, Queue<Frame>* audio_out_q
) :
    video_in_q(video_in_q),
    audio_in_q(audio_in_q),
    video_out_q(video_out_q),
    audio_out_q(audio_out_q)
{
    ex.fnMsgOut = fnMsg;
    init(videoDecoder, audioDecoder);
}

int av::Display::initAudio(int sample_rate, AVSampleFormat sample_fmt, int channels, uint64_t channel_layout, int frame_size)
{
    int ret = 0;
    try {
        want.channels = channels;
        want.freq = sample_rate;
        want.format = AUDIO_F32;
        want.silence = 0;
        want.samples = frame_size;
        want.userdata = this;
        want.callback = AudioCallback;

        dataSize = av_samples_get_buffer_size(NULL, channels, frame_size + padding, AV_SAMPLE_FMT_FLT, 0);
        audioBuf = new uint8_t[dataSize];

        ex.ck(swr_ctx = swr_alloc(), CmdTag::SA);
        av_opt_set_channel_layout(swr_ctx, "in_channel_layout",channel_layout, 0);
        av_opt_set_channel_layout(swr_ctx, "out_channel_layout", channel_layout, 0);
        av_opt_set_int(swr_ctx, "in_sample_rate", sample_rate, 0);
        av_opt_set_int(swr_ctx, "out_sample_rate", sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", sample_fmt, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
        ex.ck(swr_init(swr_ctx), CmdTag::SI);

        if (!SDL_WasInit(SDL_INIT_AUDIO)) {
            if (SDL_Init(SDL_INIT_AUDIO))
                throw Exception(std::string("SDL audio init error: ") + SDL_GetError());
        }

        audioDeviceID = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (audioDeviceID == 0)
            throw Exception(std::string("SDL_OpenAudioDevice exception: ") + SDL_GetError());

        SDL_PauseAudioDevice(audioDeviceID, 0);
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "av::Display::initAudio exception: ");
        ret = -1;
    }
    return ret;
}

int av::Display::initVideo(int width, int height, AVPixelFormat pix_fmt)
{
    int ret = 0;
    try {
        Uint32 sdl_format = 0;
        int w = width;
        int h = height;

        switch (pix_fmt) {
        case AV_PIX_FMT_YUV420P:
            sdl_format = SDL_PIXELFORMAT_IYUV;
            break;
        case AV_PIX_FMT_RGB24:
            sdl_format = SDL_PIXELFORMAT_RGB24;
            break;
        case AV_PIX_FMT_RGBA:
            sdl_format = SDL_PIXELFORMAT_RGBA32;
            break;
        case AV_PIX_FMT_BGR24:
            sdl_format = SDL_PIXELFORMAT_BGR24;
            break;
        case AV_PIX_FMT_BGRA:
            sdl_format = SDL_PIXELFORMAT_BGRA32;
            break;
        default:
            throw Exception("unsupported pix fmt");
        }

        if (!SDL_WasInit(SDL_INIT_VIDEO)) {
            if (SDL_Init(SDL_INIT_VIDEO))
                throw Exception(std::string("SDL video init error: ") + SDL_GetError());
        }

        if (!window) {
            const char* pix_fmt_name = av_get_pix_fmt_name(pix_fmt);
            std::stringstream str;
            str << "initializing video display | width: " << width << " height: " << height
                << " pixel format: " << pix_fmt_name ? pix_fmt_name : "unknown pixel format";
            ex.msg(str.str());

            window = SDL_CreateWindow("window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 0);
            if (!window)
                throw Exception(std::string("SDL_CreateWindow") + SDL_GetError());
            renderer = SDL_CreateRenderer(window, -1, 0);
            if (!renderer)
                throw Exception(std::string("SDL_CreateRenderer") + SDL_GetError());
            texture = SDL_CreateTexture(renderer, sdl_format, SDL_TEXTUREACCESS_STREAMING, w, h);
            if (!texture)
                throw Exception(std::string("SDL_CreateTexture") + SDL_GetError());
        }
        else {
            int window_width;
            int window_height;
            SDL_GetWindowSize(window, &window_width, &window_height);
            if (!(window_width == w && window_height == h)) {
                SDL_SetWindowSize(window, w, h);
                SDL_DisplayMode DM;
                SDL_GetCurrentDisplayMode(0, &DM);
                auto Width = DM.w;
                auto Height = DM.h;
                int x = (Width - w) / 2;
                int y = (Height - h) / 2;
                SDL_SetWindowPosition(window, x, y);
                if (texture)
                    SDL_DestroyTexture(texture);
                texture = SDL_CreateTexture(renderer, sdl_format, SDL_TEXTUREACCESS_STREAMING, w, h);
            }
        }
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "av::Display::initVideo exception: ");
        ret = -1;
    }
    return ret;
}

void av::Display::init(const Decoder& videoDecoder, const Decoder& audioDecoder)
{
    AVCodecContext* v = videoDecoder.dec_ctx;
    if (!video_in_q) video_in_q = videoDecoder.frame_q;
    AVCodecContext* a = audioDecoder.dec_ctx;
    if (!audio_in_q) audio_in_q = audioDecoder.frame_q;

    try {
        double factor = av_q2d(videoDecoder.stream->time_base);
        std::cout << "time base " << videoDecoder.stream->time_base.num << " / " << videoDecoder.stream->time_base.den << std::endl;
        start_time = videoDecoder.stream->start_time * factor * 1000;
        duration = videoDecoder.stream->duration * factor * 1000;
        ex.ck(initAudio(a->sample_rate, a->sample_fmt, a->channels, a->channel_layout, a->frame_size));
        //ex.ck(initVideo(v->width, v->height, v->pix_fmt));
    }
    catch (const Exception& e) {
        ex.msg(e.what(), MsgPriority::CRITICAL, "Display constructor exception: ");
    }

}

av::Display::~Display()
{
    close();
    SDL_PauseAudioDevice(audioDeviceID, true);
    if (audioBuf) delete[] audioBuf;
}

void av::Display::close()
{
    if (texture)  SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    SDL_Quit();
}

bool av::Display::isPaused()
{
    return paused;
}

void av::Display::togglePause()
{
    paused = !paused;
    SDL_PauseAudioDevice(audioDeviceID, paused);
    rtClock.pause(paused);
}

std::string av::Display::audioDeviceStatus()
{
    std::stringstream str;
    bool output = false;
    int count = SDL_GetNumAudioDevices(output);
    for (int i = 0; i < count; i++)
        str << "audio device: " << i << " name: " << SDL_GetAudioDeviceName(i, output) << "\n";

    str << "selected audio device ID (+2): " << audioDeviceID;
    str << "CHANNELS  want: " << (int)want.channels << "\n          have: " << (int)have.channels;
    str << "FREQUENCY want: " << want.freq << "\n          have: " << have.freq;
    str << "SAMPLES   want: " << want.samples << "\n          have: " << have.samples;
    return str.str();
}

bool av::Display::display()
{
    bool playing = true;
    Frame f;
    EventLoopState state;

    while (true)
    {
        uint64_t result = rtClock.milliseconds();
        state = eventLoop.loop();
        if (state == EventLoopState::QUIT) {
            playing = false;
            break;
        }
        else if (state == EventLoopState::PAUSE) {
            togglePause();
            break;
        }

        if (paused) {
            SDL_Delay(SDL_EVENT_LOOP_WAIT);
            break;
        }

        if (!video_in_q) {
            SDL_Delay(SDL_EVENT_LOOP_WAIT);
            break;
        }
 
        try {
            video_in_q->pop(f);
            if (video_out_q) video_out_q->push(f);

            if (f.isValid()) {
                handleEvent(eventLoop.last_event, f);
                ex.ck(initVideo(f.m_frame->width, f.m_frame->height, (AVPixelFormat)f.m_frame->format), "initVideo");

                if (f.m_frame->format == AV_PIX_FMT_YUV420P) {
                    ex.ck(SDL_UpdateYUVTexture(texture, NULL,
                        f.m_frame->data[0], f.m_frame->linesize[0],
                        f.m_frame->data[1], f.m_frame->linesize[1],
                        f.m_frame->data[2], f.m_frame->linesize[2]),
                        std::string("SDL_UpdateYUVTexture") + SDL_GetError());
                }
                else {
                    ex.ck(SDL_UpdateTexture(texture, NULL, f.m_frame->data[0], f.m_frame->linesize[0]),
                        std::string("SDL_UpdateTexture") + SDL_GetError());
                }

                SDL_RenderClear(renderer);
                ex.ck(SDL_RenderCopy(renderer, texture, NULL, NULL), 
                    std::string("SDL_renderCopy") + SDL_GetError());
                SDL_RenderPresent(renderer);

                uint64_t diff = rtClock.update(f.m_rts);

                if (diff > 0)
                    SDL_Delay(diff);

            }
            else {
                ex.msg("Display receive null eof");
                state = EventLoopState::EOF;
                playing = false;
                break;
            }
        }
        catch (const QueueClosedException& e) {
            state = EventLoopState::QUIT;
            playing = false;
            ex.msg(e.what(), MsgPriority::INFO, "Display::display exception: ");
            break;
        }
        catch (const Exception& e) {
            state = EventLoopState::QUIT;
            ex.msg(e.what(), MsgPriority::CRITICAL, "Display::display exception: ");
            ex.msg(std::string("last frame description: ") + f.description());
            playing = false;
            break;
        }
    }

    if (state == EventLoopState::QUIT || state == EventLoopState::EOF)
        close();

    return playing;
}

void av::Display::AudioCallback(void* userdata, uint8_t* stream, int len)
{
    Display* d = (Display*)userdata;

    try {
        memset(stream, 0, len);
        while (len > 0) {
            if (d->audioBufIndex >= d->audioBufSize) {
                d->audioBufSize = 0;
                d->audioBufIndex = 0;

                d->audio_in_q->pop(d->tmp);
                if (d->audio_out_q) d->audio_out_q->push(d->tmp);

                if (d->tmp.isValid()) {
                    int diff = d->rtClock.sync(d->tmp.m_rts);
                    d->audioBufSize = av_samples_get_buffer_size(NULL, d->channels(), d->nb_samples() + d->padding, AV_SAMPLE_FMT_FLT, 0);
                    if (d->audioBufSize == d->dataSize) {
                        swr_convert(d->swr_ctx, &d->audioBuf, d->nb_samples(), d->data(), d->nb_samples());
                        int remainder = d->audioBufSize - d->audioBufIndex;
                        if (remainder > len) remainder = len;
                        if (remainder > 0) memcpy(stream, d->audioBuf + d->audioBufIndex, remainder);

                        len -= remainder;
                        stream += remainder;
                        d->audioBufIndex += remainder;
                    }
                    else {
                        std::stringstream str;
                        str << "Inconsistent data sizes audioBufSize: " << d->audioBufSize << " dataSize: " << d->dataSize;
                        d->ex.msg(str.str());
                    }
                }
                else {
                    SDL_PauseAudioDevice(d->audioDeviceID, true);
                    len = -1;
                    d->ex.msg("audio callback received eof");
                }
            }
        }
    }
    catch (const QueueClosedException& e) { }
}

void av::Display::handleEvent(const SDL_Event& e, Frame& f) {
    if (e.type == SDL_MOUSEMOTION) {

        cv::Mat mat = f.mat();
        if (!mat.empty()) {
            cv::Mat menu = mat.clone();
            float menu_height = 0.15;
            for (int i = menu.rows * (1 - menu_height); i < menu.rows; i++) {
                cv::Mat r = menu.row(i);
                r = cv::Scalar(0, 0, 0);
            }

            SDL_Event e = eventLoop.last_event;
            Sint32 mouse_x = e.motion.x;
            Sint32 mouse_y = e.motion.y;

            cv::Mat matPlay = cv::imread("../../data/play.png");
            int x_offset = 100;
            int y_offset = menu.rows * (1 - menu_height) + 20;
            int x_end = x_offset + matPlay.cols;
            int y_end = y_offset + matPlay.rows;


            cv::Scalar bar_color(178, 178, 178);
            cv::Scalar elapsed_color(178, 178, 0);
            int start_x = menu.cols * 0.1;
            int end_x = menu.cols * 0.9;
            float pctg_elapsed = f.m_rts / (float)duration;
            int elapsed_x = (end_x - start_x) * pctg_elapsed + start_x;
            int loc_y = menu.rows * 0.95;
            int bar_width = 5;
            int bar_lo = loc_y - bar_width / 2.0f - 4;
            int bar_hi = loc_y + bar_width / 2.0f + 4;

            if (mouse_x > start_x &&
                mouse_x < end_x   &&
                mouse_y < bar_hi  &&
                mouse_y > bar_lo)
            {
                bar_color = cv::Scalar(255, 255, 255);
                elapsed_color = cv::Scalar(255, 255, 0);
            }

            cv::Point bar_start(start_x, loc_y);
            cv::Point bar_end(end_x, loc_y);
            cv::Point elapsed(elapsed_x, loc_y);
            cv::line(menu, bar_start, bar_end, bar_color, bar_width);
            cv::line(menu, bar_start, elapsed, elapsed_color, bar_width);
            int base = 0;
            auto font = cv::FONT_HERSHEY_SIMPLEX;
            float font_size = 0.5f;
            cv::String str_elapsed = std::to_string(f.m_rts);
            cv::Size text_size = cv::getTextSize(str_elapsed, font, font_size, 1, &base);

            int text_x = start_x - text_size.width - 10;
            int text_y = loc_y + text_size.height / 2.0f;

            cv::putText(menu, std::to_string(f.m_rts), cv::Point(text_x, text_y),
                font, font_size, bar_color, 1);
            
            float alpha = 0.5;
            cv::addWeighted(menu, alpha, mat, 1 - alpha, 0, mat);
        }
    }
}