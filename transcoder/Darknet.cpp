#include "Darknet.h"

av::Darknet::Darknet
(
	const std::string& cfg_filename,
	const std::string& weights_filename,
	const std::string& names_filename,
	Queue<Frame>* frame_out_q
) :
	cfg_filename(cfg_filename),
	weights_filename(weights_filename),
	names_filename(names_filename),
	frame_out_q(frame_out_q)
{
	detector = new Detector(cfg_filename, weights_filename);
}

void av::Darknet::detect(Frame* f)
{
	f->m_detections = detector->detect(f->mat());
}