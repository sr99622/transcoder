#pragma once

#include "..\Utilities\avexception.h"

class FileReader
{
public:
	FileReader(const char* filename);
	~FileReader();
	AVPacket* read_packet();

	AVFormatContext* fmt_ctx;
	int video_stream_index = -1;
	int audio_stream_index = -1;

	AVExceptionHandler av;
};

