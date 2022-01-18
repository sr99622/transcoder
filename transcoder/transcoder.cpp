#include <iostream>
#include "Factory.h"
#include "Display.h"


int main(int argc, char** argv)
{

    if (argc < 2) {
        std::cout << "Usage: demuxing <filename> <hardware decoder name> (*optional)\n"
            << "Available hardware decoders are listed below" << std::endl;
        
        AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            std::cout << av_hwdevice_get_type_name(type) << std::endl;


        std::cout << "\nSsample run commands to view output" << std::endl;
        std::cout << "ffplay -f rawvideo -pixel_format yuv420p -video_size 640x480 video" << std::endl;
        std::cout << "ffplay -f rawvideo -pixel_format nv12 -video_size 640x480 video" << std::endl;
        std::cout << "ffplay -f f32le -ac 1 -ar 44100 audio" << std::endl;
        return 0;
    }

    const char* hw_dec_name = "none";
    if (argc >= 3)
        hw_dec_name = argv[2];

    std::cout << "hw_dec_name: " << hw_dec_name << std::endl;

    AVHWDeviceType hw_dev_type = av_hwdevice_find_type_by_name(hw_dec_name);
    if (hw_dev_type != AV_HWDEVICE_TYPE_NONE)
        std::cout << "hw decoder: " << av_hwdevice_get_type_name(hw_dev_type) << std::endl;
    else
        std::cout << "using cpu decoder" << std::endl;

    const char* cmd = NULL;
    if (argc == 4)
        cmd = argv[3];
    if (cmd)
        std::cout << "filter: " << cmd << std::endl;
    else
        std::cout << "NO FILTER" << std::endl;

    const char* filename = "test.mp4";
    av::StreamParameters params;
    params.format = "mp4";
    params.pix_fmt = AV_PIX_FMT_YUV420P;
    params.width = 1920;
    params.height = 1080;
    params.frame_rate = 25;
    params.video_time_base = av_make_q(1, params.frame_rate);
    params.video_bit_rate = 1200000;
    params.gop_size = 12;
    params.sample_fmt = AV_SAMPLE_FMT_FLTP;
    params.audio_bit_rate = 64000;
    params.sample_rate = 44100;
    params.audio_time_base = av_make_q(1, params.sample_rate);
    params.channel_layout = AV_CH_LAYOUT_STEREO;
    params.channels = 2;
    params.nb_samples = 1024;
    params.audio_codec_name = "aac";
    params.hw_device_type = AV_HWDEVICE_TYPE_CUDA;
    params.hw_pix_fmt = AV_PIX_FMT_CUDA;
    params.sw_pix_fmt = AV_PIX_FMT_YUV420P;
    params.video_codec_name = "h264_nvenc";
    params.profile = "high";

    av::Factory avf;

    av::FileWriter writer(params);

    av::CircularQueue<AVPacket*> vpq_encoder(10);
    av::CircularQueue<AVPacket*> apq_encoder(10);

    av::Encoder videoStream(&writer, params, &vpq_encoder, AVMEDIA_TYPE_VIDEO);
    av::Encoder audioStream(&writer, params, &apq_encoder, AVMEDIA_TYPE_AUDIO);

    const char* src_filename = argv[1];
    av::FileReader reader(src_filename, avf.msg);

    av::CircularQueue<AVPacket*> vpq_reader(10);
    av::CircularQueue<AVPacket*> apq_reader(10);

    av::FrameQueueMonitor fqm(true, true);
    //av::CircularQueue<av::Frame> video_frame_q(10, "V", fqm.mntrAction, nullptr, avf.msgOut);
    //av::CircularQueue<av::Frame> audio_frame_q(10, "A", fqm.mntrAction, nullptr, avf.msgOut);
    av::CircularQueue<av::Frame> vfq_decoder(10);
    av::CircularQueue<av::Frame> afq_decoder(10);

    
    //av::CircularQueue<av::Frame> avfq_muxer(10, "I", fqm.mntrAction, nullptr, avf.msgOut);

    av::Decoder video_decoder(reader.fmt_ctx, reader.video_stream_index, &vfq_decoder, avf.msg, hw_dev_type);
    av::Decoder audio_decoder(reader.fmt_ctx, reader.audio_stream_index, &afq_decoder, avf.msg);


    std::thread file_reading(avf.read, &reader, &vpq_reader, &apq_reader);

    //std::thread video_pkt_draining(avf.pkt_drain, &vpq_reader);
    //std::thread audio_pkt_draining(avf.pkt_drain, &apq_reader);

    std::thread video_decoding(avf.decode, &video_decoder, &vpq_reader);
    std::thread audio_decoding(avf.decode, &audio_decoder, &apq_reader);

    //av::CircularQueue<av::Frame> vfq_filter(10, "F", fqm.mntrAction, nullptr, avf.msgOut);
    av::CircularQueue<av::Frame> vfq_filter(10);
    av::Filter video_filter(video_decoder, cmd, &vfq_filter);
    std::thread video_filtering(avf.filter, &video_filter, &vfq_decoder);

    //std::thread muxing(avf.mux, &vfq_decoder, &afq_decoder, &avfq_muxer);
    //std::thread mux_draining(avf.drain, &avfq_muxer);

    //av::CircularQueue<av::Frame> vfq_display(10, "V", fqm.mntrAction, nullptr, avf.msgOut);
    //av::CircularQueue<av::Frame> afq_display(10, "A", fqm.mntrAction, nullptr, avf.msgOut);
    av::CircularQueue<av::Frame> vfq_display(10);
    av::CircularQueue<av::Frame> afq_display(10);

    //std::thread video_draining(avf.drain, &vfq_decoder);
    //std::thread audio_draining(avf.drain, &afq_decoder);

    bool use_encoder = false;

    av::Display display;

    if (use_encoder) {
        display.video_in_q = &vfq_decoder;
        display.audio_in_q = &afq_decoder;
        display.video_out_q = &vfq_display;
        display.audio_out_q = &afq_display;
        display.ex.fnMsgOut = avf.msg;
        display.init(video_decoder, audio_decoder);
        writer.open(filename);
    }
    else {
        //display.video_in_q = &vfq_decoder;
        display.video_in_q = &vfq_filter;
        display.audio_in_q = &afq_decoder;
        display.video_out_q = nullptr;
        display.audio_out_q = nullptr;
        display.ex.fnMsgOut = avf.msg;
        display.init(video_decoder, audio_decoder);
    }


    /* encoder */
    std::thread video_encoding;
    std::thread audio_encoding;
    std::thread video_file_writing;
    std::thread audio_file_writing;

    if (use_encoder) {
        video_encoding = std::thread(avf.encode, &videoStream, &vfq_display);
        audio_encoding = std::thread(avf.encode, &audioStream, &afq_display);
        video_file_writing = std::thread(avf.write, &writer, &vpq_encoder);
        audio_file_writing = std::thread(avf.write, &writer, &apq_encoder);
    }

    while (display.display()) {}

    avf.msg("main program left sdl event loop");
    vpq_reader.close();
    apq_reader.close();
    vfq_decoder.close();
    afq_decoder.close();
    vfq_filter.close();

    /* encoder */
    if (use_encoder) {
        vfq_display.close();
        afq_display.close();
        vpq_encoder.close();
        apq_encoder.close();
    }
 
    file_reading.join();
    //video_pkt_draining.join();
    //audio_pkt_draining.join();
    video_decoding.join();
    audio_decoding.join();
    //video_draining.join();
    //audio_draining.join();
    //muxing.join();
    //mux_draining.join();
    video_filtering.join();

    /* encoder */
    if (use_encoder) {
        video_encoding.join();
        audio_encoding.join();
        video_file_writing.join();
        audio_file_writing.join();
        writer.close();
    }

    avf.msg("Demuxing succeeded.");

    return 0;
}
