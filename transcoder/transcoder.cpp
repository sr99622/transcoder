#include "Factory.h"

int test(int argc, char** argv) 
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

    const char* out_filename = "test.mp4";
    av::StreamParameters params;
    params.format = "mp4";
    params.video_codec_name = "h264";
    params.pix_fmt = AV_PIX_FMT_YUV420P;
    params.width = 1280;
    params.height = 720;
    params.frame_rate = 25;
    params.video_time_base = av_make_q(1, params.frame_rate);
    params.video_bit_rate = 120000000;
    params.gop_size = 12;
    params.sample_fmt = AV_SAMPLE_FMT_FLTP;
    params.audio_bit_rate = 64000;
    params.sample_rate = 44100;
    params.audio_time_base = av_make_q(1, params.sample_rate);
    params.channel_layout = AV_CH_LAYOUT_STEREO;
    params.channels = 2;
    params.nb_samples = 1024;
    params.audio_codec_name = "aac";
    params.profile = "high";

    std::string cfg = "C:/Users/sr996/models/reduced/ami1/yolov4.cfg";
    std::string weights = "C:/Users/sr996/models/reduced/ami1/yolov4.weights";
    std::string names = "C:/Users/sr996/models/reduced/ami1/coco.names";
    const char* model_dir = "C:/Users/sr996/Downloads/ssd_mobilenet_v2_320x320_coco17_tpu-8/saved_model";

    bool use_encoder = false;
    bool use_hardware = false;

    bool use_filter = true;
    bool use_yo_detector = true;
    bool use_py_detector = false;
    bool use_tracker = true;

    if (use_hardware) {
        params.hw_device_type = AV_HWDEVICE_TYPE_CUDA;
        params.hw_pix_fmt = AV_PIX_FMT_CUDA;
        params.sw_pix_fmt = AV_PIX_FMT_YUV420P;
        params.video_codec_name = "h264_nvenc";
    }

    av::Factory avf;
    av::FrameQueueMonitor fqm;

    const char* src_filename = argv[1];
    av::FileReader reader(src_filename, avf.msg);

    av::Queue<AVPacket*> vpq_reader(10);
    av::Queue<AVPacket*> apq_reader(10);

    av::Queue<av::Frame> vfq_decoder(10);
    //av::Queue<av::Frame> vfq_decoder(10, "IN ", fqm.mntrAction, nullptr, avf.msgOut);
    av::Queue<av::Frame> afq_decoder(10);

    av::Decoder video_decoder(reader.fmt_ctx, reader.video_stream_index, &vfq_decoder, avf.msg, hw_dev_type);
    av::Decoder audio_decoder(reader.fmt_ctx, reader.audio_stream_index, &afq_decoder, avf.msg);

    std::thread file_reading(avf.read, &reader, &vpq_reader, &apq_reader);

    std::thread video_decoding(avf.decode, &video_decoder, &vpq_reader);
    std::thread audio_decoding(avf.decode, &audio_decoder, &apq_reader);

    std::thread video_filtering;
    av::Queue<av::Frame> vfq_filter(10);
    //av::Queue<av::Frame> vfq_filter(10, "OUT", fqm.mntrAction, nullptr, avf.msgOut);
    av::Filter video_filter(video_decoder, cmd, &vfq_filter);

    if (use_filter)
        video_filtering = std::thread(avf.filter, &video_filter, &vfq_decoder);

    std::thread py_detecting;
    av::Queue<av::Frame> vfq_detector(10);
    std::thread yo_detecting;
    av::Darknet yo_detector;

    if (use_py_detector && use_yo_detector)
        avf.msg("ERROR CANNOT USE BOTH YOLO AND PY SSD DETECTOR AT THE SAME TIME");

    if (use_py_detector) {
        if (use_filter)
            py_detecting = std::thread(avf.py_detect, &vfq_detector, &vfq_filter);
        else
            py_detecting = std::thread(avf.py_detect, &vfq_detector, &vfq_decoder);
    }

    if (use_yo_detector) {

        yo_detector = av::Darknet(cfg, weights, names, &vfq_detector);

        if (use_filter)
            yo_detecting = std::thread(avf.detect, &yo_detector, &vfq_filter);
        else
            yo_detecting = std::thread(avf.detect, &yo_detector, &vfq_decoder);
    }

    std::thread tracking;
    av::Queue<av::Frame> vfq_tracker(10);

    if (use_tracker) {
        tracking = std::thread(avf.track, &vfq_tracker, &vfq_detector);
    }

    av::Queue<av::Frame> vfq_display(10);
    av::Queue<av::Frame> afq_display(10);

    av::FileWriter writer(params);

    av::Queue<AVPacket*> vpq_encoder(10);
    av::Queue<AVPacket*> apq_encoder(10);

    av::Encoder video_encoder(&writer, params, &vpq_encoder, AVMEDIA_TYPE_VIDEO);
    av::Encoder audio_encoder(&writer, params, &apq_encoder, AVMEDIA_TYPE_AUDIO);

    av::Display display;

    if (use_encoder) {

        if (use_yo_detector || use_py_detector) {
            if (use_tracker)
                display.video_in_q = &vfq_tracker;
            else
                display.video_in_q = &vfq_detector;
        }
        else {
            if (use_filter)
                display.video_in_q = &vfq_filter;
            else
                display.video_in_q = &vfq_decoder;
        }

        display.audio_in_q = &afq_decoder;
        display.video_out_q = &vfq_display;
        display.audio_out_q = &afq_display;
        display.ex.fnMsgOut = avf.msg;
        display.init(video_decoder, audio_decoder);
        writer.open(out_filename);

    }
    else {

        if (use_yo_detector || use_py_detector) {
            if (use_tracker)
                display.video_in_q = &vfq_tracker;
            else
                display.video_in_q = &vfq_detector;
        }
        else {
            if (use_filter)
                display.video_in_q = &vfq_filter;
            else
                display.video_in_q = &vfq_decoder;
        }

        display.audio_in_q = &afq_decoder;
        display.video_out_q = nullptr;
        display.audio_out_q = nullptr;
        display.ex.fnMsgOut = avf.msg;
        display.init(video_decoder, audio_decoder);

    }

    std::thread video_encoding;
    std::thread audio_encoding;
    std::thread video_file_writing;
    std::thread audio_file_writing;

    if (use_encoder) {
        video_encoding = std::thread(avf.encode, &video_encoder, &vfq_display);
        audio_encoding = std::thread(avf.encode, &audio_encoder, &afq_display);
        video_file_writing = std::thread(avf.write, &writer, &vpq_encoder);
        audio_file_writing = std::thread(avf.write, &writer, &apq_encoder);
    }

    while (display.display()) {}

    avf.msg("main program left sdl event loop");

    vpq_reader.close();
    apq_reader.close();
    vfq_decoder.close();
    afq_decoder.close();

    if (use_filter)
        vfq_filter.close();

    if (use_yo_detector || use_py_detector) 
        vfq_detector.close();

    if (use_tracker)
        vfq_tracker.close();

    if (use_encoder) {
        vfq_display.push(av::Frame(nullptr));
        afq_display.push(av::Frame(nullptr));
    }

    file_reading.join();
    video_decoding.join();
    audio_decoding.join();

    if (use_filter)
        video_filtering.join();

    if (use_yo_detector)
        yo_detecting.join();

    if (use_py_detector)
        py_detecting.join();

    if (use_tracker)
        tracking.join();

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
int main(int argc, char** argv)
{
    /*
    Py_Initialize();
    import_array();
    if (PyErr_Occurred())
        std::cout << "PyErr" << std::endl;

    if (Py_IsInitialized())
        std::cout << "PYTHON INITIALIZED" << std::endl;
    else
        std::cout << "PYTHON NOT INITIALIZED" << std::endl;
    */


    return test(argc, argv);

    //Py_Finalize();
}




/*
    av::StreamParameters params;
    params.format = "mp4";
    params.video_codec_name = "h264";
    params.pix_fmt = AV_PIX_FMT_YUV420P;
    params.width = 1280;
    params.height = 720;
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
    params.profile = "high";
*/



//av::Queue<av::Frame> video_frame_q(10, "V", fqm.mntrAction, nullptr, avf.msgOut);
//av::Queue<av::Frame> audio_frame_q(10, "A", fqm.mntrAction, nullptr, avf.msgOut);
    //av::Queue<av::Frame> avfq_muxer(10, "I", fqm.mntrAction, nullptr, avf.msgOut);

    //std::thread video_pkt_draining(avf.pkt_drain, &vpq_reader);
    //std::thread audio_pkt_draining(avf.pkt_drain, &apq_reader);

    //av::Queue<av::Frame> vfq_filter(10, "F", fqm.mntrAction, nullptr, avf.msgOut);
    //std::thread muxing(avf.mux, &vfq_decoder, &afq_decoder, &avfq_muxer);
    //std::thread mux_draining(avf.drain, &avfq_muxer);

    //av::Queue<av::Frame> vfq_display(10, "V", fqm.mntrAction, nullptr, avf.msgOut);
    //av::Queue<av::Frame> afq_display(10, "A", fqm.mntrAction, nullptr, avf.msgOut);
    //std::thread video_draining(avf.drain, &vfq_decoder);
    //std::thread audio_draining(avf.drain, &afq_decoder);

    //video_pkt_draining.join();
    //audio_pkt_draining.join();

    //video_draining.join();
    //audio_draining.join();
    //muxing.join();
    //mux_draining.join();
