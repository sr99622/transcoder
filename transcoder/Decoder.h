#pragma once

#include "Exception.h"
#include "RawFileWriter.h"
#include "CircularQueue.h"
#include "Frame.h"

namespace av
{

class Decoder
{
public:
	Decoder(AVFormatContext* fmt_ctx, int stream_index,
		CircularQueue<Frame>* frame_q,
		std::function<void(const std::string&, MsgPriority, const std::string&)> fnMsg = nullptr,
		AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE);

	~Decoder();
	int decode(AVPacket* pkt);
	void close();

	std::string streamInfo();
	std::string hardwareInfo();

	//Frame tmp;
	AVFrame* frame = NULL;
	AVFrame* sw_frame = NULL;
	AVFrame* cvt_frame = NULL;
	AVStream* stream = NULL;
	AVCodec* dec = NULL;
	AVCodecContext* dec_ctx = NULL;
	AVBufferRef* hw_device_ctx = NULL;
	SwsContext* sws_ctx = NULL;

	CircularQueue<Frame>* frame_q;

	ExceptionHandler ex;
};

}

