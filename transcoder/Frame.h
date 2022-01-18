#pragma once

#include "Exception.h"

namespace av
{

class Frame
{
public:
	Frame();
	~Frame();
	Frame(const Frame& other);
	Frame(Frame&& other) noexcept;
	Frame(AVFrame* src);
	Frame& operator=(const Frame& other);
	Frame& operator=(Frame&& other) noexcept;
	AVFrame* copyFrame(AVFrame* src);
	bool isValid() const { return m_frame ? true : false; }
	void invalidate();
	AVMediaType mediaType();
	uint64_t pts() { return m_frame ? m_frame->pts : AV_NOPTS_VALUE; }
	void set_rts(AVStream* stream);  // callback from Decoder::decode
	std::string description();

	AVFrame* m_frame = NULL;
	uint64_t m_rts;
	bool show = false;

};

class FrameQueueMonitor
{
public:
	FrameQueueMonitor();
	FrameQueueMonitor(bool showPush, bool showPop);
	static std::string pad(int full, int taken);
	static std::string mntrAction(Frame&, int, bool, std::string&);
	static void mntrWait(bool, bool, std::string&);
};


}

