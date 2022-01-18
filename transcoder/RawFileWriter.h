#pragma once

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
}

#include "Exception.h"
#include "Frame.h"

namespace av
{

class RawFileWriter
{
public:
	RawFileWriter(AVCodecContext* codec_ctx);
	~RawFileWriter();

	int write_frame(const Frame& f);
	int write_frame(AVFrame* frame);
	int output_video_frame(AVFrame* frame);
	int output_audio_frame(AVFrame* frame);

	int width;
	int height;
	AVPixelFormat pix_fmt;
	AVMediaType type;

	uint8_t* buffer = NULL;
	int size;

	const char* filename;
	FILE* file;
	int frame_count = 0;

	ExceptionHandler ex;
};

}

