#pragma once

#include "../Utilities/avexception.h"
#include "RawFileWriter.h"
#include "CircularQueue.h"
#include "Frame.h"

class Decoder
{
public:
	Decoder(AVFormatContext* fmt_ctx, int stream_index, CircularQueue<Frame>* q, AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE);
	~Decoder();
	int decode_packet(AVPacket* pkt);
	void adjust_pts(AVFrame* frame);
	void flush();

	AVFrame* frame = NULL;
	AVFrame* sw_frame = NULL;
	AVCodecContext* dec_ctx = NULL;
	AVBufferRef* hw_device_ctx = NULL;
	CircularQueue<Frame>* frame_q;
	AVExceptionHandler av;
	Frame tmp;
	AVRational next_pts_tb;
	int64_t next_pts;
};

