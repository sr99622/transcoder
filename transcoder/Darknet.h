#pragma once

#include "Queue.h"
#include "Frame.h"

#include "yolo_v2_class.hpp"

namespace av
{

class Darknet
{
public:

	Darknet() {}
	Darknet
	(
		const std::string& cfg_filename,
		const std::string& weights_filename,
		const std::string& names_filename,
		Queue<Frame>* frame_out_q
	);

	~Darknet() { delete detector; }

	void detect(Frame* f);

	std::string cfg_filename;
	std::string weights_filename;
	std::string names_filename;

	Queue<Frame>* frame_out_q;

	Detector* detector;
	std::vector<bbox_t> detections;
};

}


