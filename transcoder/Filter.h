#pragma once

extern "C" {
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

#include "Exception.h"
#include "Decoder.h"

namespace av
{

class Filter
{
public:
	Filter(const Decoder& decoder, const char* description, CircularQueue<Frame>* frame_out_q);
	~Filter();

	int filter(const Frame& f);

	AVFilterContext* sink_ctx;
	AVFilterContext* src_ctx;
	AVFilterGraph* graph;
	AVFrame* frame;
	Frame tmp;
	AVStream* stream;
	CircularQueue<Frame>* frame_out_q;
	ExceptionHandler ex;

};

}

