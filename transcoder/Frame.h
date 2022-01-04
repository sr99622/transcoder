#pragma once

#include "../Utilities/avexception.h"

class Frame
{
public:
	Frame() : frame(av_frame_alloc()) { }
	~Frame() { av_frame_free(&frame); }
	bool isValid() const { return frame ? true : false; }
	void invalidate() { av_frame_free(&frame); }

	Frame(const Frame& other) : frame(copyFrame(other.frame)) 
	{	
		//std::cout << "copy constructor" << std::endl;
	}

	/*
	Frame(const Frame& other)
	{
		frame = copyFrame(other.frame);
	}
	*/

	Frame(AVFrame* src)
	{
		frame = copyFrame(src);
	}

	Frame& operator=(const Frame& other)
	{
		bool show = false;
		if (show) std::cout << "assignment operator start" << std::endl;
		if (other.isValid()) {
			if (sameSizeAs(other)) {
				if (show) std::cout << "assignment operator sameSizeAs" << std::endl;
				//swap(*this, other);
				av_frame_ref(frame, other.frame);
			}
			else {
				if (show) std::cout << "assignment operator new tmp" << std::endl;
				if (frame)
					av_frame_free(&frame);
				frame = av_frame_clone(other.frame);
				/*
				Frame tmp(other);
				if (!empty())
					av_frame_free(&frame);
				swap(*this, tmp);
				*/
			}
		}
		else {
			invalidate();
		}
		if (show) std::cout << "assignment operator end" << std::endl;
		return *this;
	}

	AVFrame* copyFrame(AVFrame* src)
	{
		if (!src)
			return NULL;

		AVFrame* dst = av_frame_alloc();
		dst->format = src->format;
		dst->channel_layout = src->channel_layout;
		dst->sample_rate = src->sample_rate;
		dst->nb_samples = src->nb_samples;
		dst->width = src->width;
		dst->height = src->height;
		av_frame_get_buffer(dst, 0);
		av_frame_make_writable(dst);
		av_frame_copy_props(dst, src);
		av_frame_copy(dst, src);

		return dst;
	}

	void swap(Frame& dst, const Frame& src)
	{
		dst.frame->format = src.frame->format;
		dst.frame->channel_layout = src.frame->channel_layout;
		dst.frame->sample_rate = src.frame->sample_rate;
		dst.frame->nb_samples = src.frame->nb_samples;
		dst.frame->width = src.frame->width;
		dst.frame->height = src.frame->height;
		av_frame_copy_props(dst.frame, src.frame);
		av_frame_copy(dst.frame, src.frame);
	}

	bool empty()
	{
		if (frame->format != -1) return false;
		if (frame->width > 0
			|| frame->height > 0
			|| frame->nb_samples > 0
			|| frame->channel_layout > 0
			|| frame->sample_rate > 0) return false;

		return true;
	}

	bool sameSizeAs(const Frame& other)
	{
		if (other.isValid()) {
			if (isValid()) {
				if (frame->format != other.frame->format) return false;
				if (frame->width != other.frame->width) return false;
				if (frame->height != other.frame->height) return false;
				if (frame->channel_layout != other.frame->channel_layout) return false;
				if (frame->sample_rate != other.frame->sample_rate) return false;
				if (frame->nb_samples != other.frame->nb_samples) return false;
			}
			else {
				return false;
			}
		}
		else {
			return isValid() ? false : true;
		}
		return true;
	}

	AVFrame* frame;
	AVExceptionHandler av;

};