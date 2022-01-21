#include "Frame.h"
#include <sstream>

bool showPush;
bool showPop;

av::Frame::Frame() : 
	m_frame(NULL),
	m_rts(0)
{
	if (show) std::cout << "default constructor" << std::endl;
}

av::Frame::Frame(const Frame& other) : 
	m_frame(copyFrame(other.m_frame)), 
	m_rts(other.m_rts)
{
	if (show) std::cout << "copy constructor" << std::endl;
}

av::Frame::Frame(Frame&& other) noexcept :
	m_frame(copyFrame(other.m_frame)),
	m_rts(other.m_rts)
{
	if (show) std::cout << "move copy constructor" << std::endl;
}

av::Frame::Frame(AVFrame* src)
{ 
	av_frame_free(&m_frame);
	m_frame = copyFrame(src);
}

av::Frame::~Frame() 
{ 
	av_frame_free(&m_frame); 
	if (show) std::cout << "destructor" << std::endl;
}

av::Frame& av::Frame::operator=(const Frame& other)
{
	if (show) std::cout << "assignment operator start: " << std::endl;
	if (other.isValid()) {
		if (show) std::cout << "assignment operator new tmp" << std::endl;
		m_rts = other.m_rts;
		av_frame_free(&m_frame);
		m_frame = av_frame_clone(other.m_frame);
		av_frame_make_writable(m_frame);
	}
	else {
		invalidate();
	}
	if (show) std::cout << "assignment operator end" << std::endl;
	return *this;
}

av::Frame& av::Frame::operator=(Frame&& other) noexcept
{
	if (show) std::cout << "move assignment operator start" << std::endl;
	if (other.isValid()) {
		if (show) std::cout << "move assignment operator new tmp" << std::endl;
		m_rts = other.m_rts;
		av_frame_free(&m_frame);
		m_frame = other.m_frame;
		av_frame_make_writable(m_frame);
		other.m_frame = NULL;
	}
	else {
		invalidate();
	}
	if (show) std::cout << "move assignment operator end" << std::endl;
	return *this;
}

void av::Frame::invalidate() 
{ 
	av_frame_free(&m_frame);
	m_frame = NULL;
	m_rts = 0;
}

cv::Mat av::Frame::mat()
{
	cv::Mat mat;
	switch (m_frame->format) {
	case AV_PIX_FMT_BGR24:
		mat = cv::Mat(m_frame->height, m_frame->width, CV_8UC3, m_frame->data[0], m_frame->linesize[0]);
		break;
	case AV_PIX_FMT_BGRA:
		mat = cv::Mat(m_frame->height, m_frame->width, CV_8UC4, m_frame->data[0], m_frame->linesize[0]);
		break;
	}

	return mat;
}

AVFrame* av::Frame::copyFrame(AVFrame* src)
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

void av::Frame::drawBox(bbox_t box)
{
	cv::Mat tmp = mat();
	cv::Rect rect(box.x, box.y, box.w, box.h);
	cv::Scalar color(0, 255, 0);
	cv::rectangle(tmp, rect, color, 1);
}

AVMediaType av::Frame::mediaType() const
{
	AVMediaType result = AVMEDIA_TYPE_UNKNOWN;
	if (isValid()) {
		if (m_frame->width > 0 && m_frame->height > 0)
			result = AVMEDIA_TYPE_VIDEO;
		else if (m_frame->nb_samples > 0 && m_frame->channels > 0)
			result = AVMEDIA_TYPE_AUDIO;
	}
	return result;
}

std::string av::Frame::description() const
{
	std::stringstream str;
	if (isValid()) {
		if (mediaType() == AVMEDIA_TYPE_VIDEO) {
			const char* pix_fmt_name = av_get_pix_fmt_name((AVPixelFormat)m_frame->format);
			str << "type: VIDEO width: " << m_frame->width << " height: " << m_frame->height
				<< " format: " << (pix_fmt_name ? pix_fmt_name : "unknown pixel format")
				<< " pts: " << m_frame->pts << " m_rts: " << m_rts;
		}
		else if (mediaType() == AVMEDIA_TYPE_AUDIO) {
			const char* sample_fmt_name = av_get_sample_fmt_name((AVSampleFormat)m_frame->format);
			str << "type: AUDIO nb_samples: " << m_frame->nb_samples << " channels: " << m_frame->channels
				<< " format: " << (sample_fmt_name ? sample_fmt_name : "unknown sample format")
				<< " pts: " << m_frame->pts << " m_rts: " << m_rts;
		}
		else {
			str << "UNKNOWN MEDIA TYPE";
		}
	}
	else {
		str << "INVALID FRAME";
	}

	return str.str();
}

void av::Frame::set_rts(AVStream* stream)
{
	if (isValid()) {
		double factor = 1000 * av_q2d(stream->time_base);
		m_rts = (pts() - stream->start_time) * factor;
	}
}

void av::Frame::set_pts(AVStream* stream)
{
	if (isValid()) {
		double factor = av_q2d(stream->time_base);
		m_frame->pts = m_rts / factor / 1000;
	}
}

av::FrameQueueMonitor::FrameQueueMonitor()
{
	showPush = true;
	showPop = true;
}

av::FrameQueueMonitor::FrameQueueMonitor(bool showPushIn, bool showPopIn)
{
	showPush = showPushIn;
	showPop = showPopIn;
}

std::string av::FrameQueueMonitor::pad(int full, int taken)
{
	std::stringstream str;
	for (int i = 0; i < taken - full; i++)
		str << " ";
	return str.str();
}

std::string av::FrameQueueMonitor::mntrAction(Frame& f, int q_size, bool push, std::string& name)
{
	std::stringstream str;

	if (showPush && push || showPop && !push) {

		const char* type = av_get_media_type_string(f.mediaType());

		uint64_t pts = f.pts();
		std::string str_q_size = std::to_string(q_size);
		str_q_size.insert(str_q_size.begin(), 4 - str_q_size.length(), ' ');
		std::string str_pts = (pts != AV_NOPTS_VALUE) ? std::to_string(pts) : "AV_NOPTS_VALUE";
		if (f.mediaType() == AVMEDIA_TYPE_AUDIO)
			str_pts.insert(str_pts.begin(), 20 - str_pts.length(), ' ');
		else
			str_pts.insert(str_pts.end(), 20 - str_pts.length(), ' ');

		str << name << " "
			<< "q size " << (f.mediaType() == AVMEDIA_TYPE_AUDIO ? "    " : "") << str_q_size
			<< (f.mediaType() == AVMEDIA_TYPE_VIDEO ? "    " : "")
			<< (push ? " push " : "  pop ")
			<< str_pts
			<< " frame rts: " << f.m_rts;
	}


	return str.str();
}