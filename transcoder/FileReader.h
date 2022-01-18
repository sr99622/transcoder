#pragma once

#include "Exception.h"

namespace av
{

class FileReader
{
public:
	FileReader(const char* filename, std::function<void(const std::string&, MsgPriority, const std::string&)> = nullptr);
	~FileReader();
	AVPacket* read();

	AVFormatContext* fmt_ctx;
	int video_stream_index = -1;
	int audio_stream_index = -1;

	ExceptionHandler ex;
};

}


