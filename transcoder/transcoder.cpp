extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "FileReader.h"
#include "Decoder.h"
#include "Encoder.h"
#include "CircularQueue.h"
#include "FileWriter.h"
#include "AudioGenerator.h"
#include "VideoGenerator.h"
#include "CircularQueue.h"
#include "Frame.h"

void encode(Encoder* encoder, CircularQueue<Frame>* frame_q)
{
    Frame frame;
    while (true)
    {
        frame_q->pop(frame);
        encoder->encodeFrame(frame);
        if (!frame.isValid())
            break;
    }
    encoder->pkt_q->push(NULL);
}

void write(FileWriter* writer, CircularQueue<AVPacket*>* pkt_q)
{
    while (AVPacket* pkt = pkt_q->pop())
    {
        writer->writeFrame(pkt);
        av_packet_free(&pkt);
    }
    writer->writeFrame(NULL);
}

void read(FileReader* reader, CircularQueue<AVPacket*>* video_pkt_q, CircularQueue<AVPacket*>* audio_pkt_q)
{
    bool dbg = false;

    while (AVPacket* pkt = reader->read_packet())
    {
        if (pkt->stream_index == reader->video_stream_index) {
            if (dbg) std::cout << "reader video_pkt_in_q size: " << video_pkt_q->size() << " push pkt pts: " << pkt->pts << std::endl;
            if (video_pkt_q)
                video_pkt_q->push(pkt);
        }
        else if (pkt->stream_index == reader->audio_stream_index) {
            if (dbg) std::cout << "reader audio_pkt_in_q size: " << audio_pkt_q->size() << " push pkt pts:         " << pkt->pts << std::endl;
            if (audio_pkt_q)
                audio_pkt_q->push(pkt);
        }
    }

    if (video_pkt_q)
        video_pkt_q->push(NULL);

    if (audio_pkt_q)
        audio_pkt_q->push(NULL);
}

void decode(Decoder* decoder, CircularQueue<AVPacket*>* pkt_q)
{
    while (AVPacket* pkt = pkt_q->pop())
    {
        decoder->decode_packet(pkt);
        av_packet_unref(pkt);
    }
    decoder->decode_packet(NULL);
    decoder->frame_q->push(Frame(NULL));
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: demuxing <filename> <hardware decoder name> (*optional)\n"
            << "Available hardware decoders are listed below" << std::endl;
        
        //show_hw_devices();
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
    if (argc == 3)
        hw_dec_name = argv[2];

    AVHWDeviceType hw_dev_type = av_hwdevice_find_type_by_name(hw_dec_name);
    if (hw_dev_type != AV_HWDEVICE_TYPE_NONE)
        std::cout << "hw decoder: " << av_hwdevice_get_type_name(hw_dev_type) << std::endl;
    else
        std::cout << "using cpu decoder" << std::endl;

    const char* filename = "test.mp4";
    StreamParameters params;
    params.format = "mp4";
    params.pix_fmt = AV_PIX_FMT_YUV420P;
    params.width = 640;
    params.height = 480;
    params.frame_rate = 25;
    params.video_time_base = av_make_q(1, params.frame_rate);
    params.video_bit_rate = 600000;
    params.gop_size = 12;
    params.sample_fmt = AV_SAMPLE_FMT_FLTP;
    params.audio_bit_rate = 64000;
    params.sample_rate = 44100;
    params.audio_time_base = av_make_q(1, params.sample_rate);
    params.channel_layout = AV_CH_LAYOUT_STEREO;
    params.channels = 2;
    params.nb_samples = 1024;
    params.audio_codec_name = "aac";
    //params.hw_device_type = AV_HWDEVICE_TYPE_CUDA;
    //params.hw_pix_fmt = AV_PIX_FMT_CUDA;
    //params.sw_pix_fmt = AV_PIX_FMT_YUV420P;
    //params.video_codec_name = "h264_nvenc";
    params.profile = "high";

    FileWriter writer(params);

    CircularQueue<AVPacket*> video_out_pkt_q(10);
    CircularQueue<AVPacket*> audio_out_pkt_q(10);

    Encoder videoStream(&writer, params, &video_out_pkt_q, AVMEDIA_TYPE_VIDEO);
    Encoder audioStream(&writer, params, &audio_out_pkt_q, AVMEDIA_TYPE_AUDIO);

    const char* src_filename = argv[1];
    FileReader reader(src_filename);

    CircularQueue<AVPacket*> video_in_pkt_q(10);
    CircularQueue<AVPacket*> audio_in_pkt_q(10);

    CircularQueue<Frame> video_frame_q(10);
    CircularQueue<Frame> audio_frame_q(10);

    Decoder video_decoder(reader.fmt_ctx, reader.video_stream_index, &video_frame_q, hw_dev_type);
    Decoder audio_decoder(reader.fmt_ctx, reader.audio_stream_index, &audio_frame_q);

    writer.openFile(filename);

    std::thread read_file(read, &reader, &video_in_pkt_q, &audio_in_pkt_q);
    std::thread decode_video(decode, &video_decoder, &video_in_pkt_q);
    std::thread decode_audio(decode, &audio_decoder, &audio_in_pkt_q);
    std::thread video_encoding(encode, &videoStream, &video_frame_q);
    std::thread audio_encoding(encode, &audioStream, &audio_frame_q);
    std::thread file_video_writing(write, &writer, &video_out_pkt_q);
    std::thread file_audio_writing(write, &writer, &audio_out_pkt_q);

    read_file.join();
    decode_video.join();
    decode_audio.join();
    video_encoding.join();
    audio_encoding.join();
    file_video_writing.join();
    file_audio_writing.join();

    writer.closeFile();

    std::cout << "Demuxing succeeded." << std::endl;

    return 0;
}
