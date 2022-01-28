#pragma once

#include "Exception.h"
#include <opencv2/opencv.hpp>
#include "yolo_v2_class.hpp"
#include "Box.h"

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
	AVMediaType mediaType() const;
	uint64_t pts() { return m_frame ? m_frame->pts : AV_NOPTS_VALUE; }
	void set_rts(AVStream* stream);  // called from Decoder::decode
	void set_pts(AVStream* stream);  // called from Encoder::encode
	std::string description() const;
	cv::Mat mat();
	void drawBox(bbox_t box);

	AVFrame* m_frame = NULL;
	uint64_t m_rts;
	std::vector<bbox_t> m_detections;
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

