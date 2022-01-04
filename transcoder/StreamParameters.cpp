#include "StreamParameters.h"

StreamParameters::StreamParameters()
{
	
}

StreamParameters::StreamParameters(AVCodecContext* ctx)
{
	if (ctx->width > 0 && ctx->height > 0) {
		readVideoCtx(ctx);
	}
	else if (ctx->channels > 0) {
		readAudioCtx(ctx);
	}
}

StreamParameters::~StreamParameters()
{
	av_dict_free(&opts);
}

void StreamParameters::readAudioCtx(AVCodecContext* ctx)
{
	sample_fmt = ctx->sample_fmt;
	if (ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = ctx->frame_size;
	channel_layout = ctx->channel_layout;
	channels = ctx->channels;
	audio_bit_rate = ctx->bit_rate;
	sample_rate = ctx->sample_rate;
	audio_time_base = ctx->time_base;
	audio_codec_name = avcodec_get_name(ctx->codec_id);
}

void StreamParameters::readVideoCtx(AVCodecContext* ctx)
{
	pix_fmt = ctx->pix_fmt;
	width = ctx->width;
	height = ctx->height;
	video_bit_rate = ctx->bit_rate;
	video_time_base = ctx->time_base;
	video_codec_name = avcodec_get_name(ctx->codec_id);
}

void StreamParameters::showAvailableOutputFormats()
{
	const AVOutputFormat* fmt = NULL;
	void* i = 0;

	while ((fmt = av_muxer_iterate(&i))) {
		std::cout << "\n" << fmt->name
			<< " - " << fmt->long_name << "\n"
			<< "    audio_codec: " << avcodec_get_name(fmt->audio_codec) << "\n"
			<< "    video_codec: " << avcodec_get_name(fmt->video_codec) << "\n";

		if (fmt->mime_type) std::cout << "    mime_type:   " << fmt->mime_type << std::endl;
		if (fmt->extensions) std::cout << "    extensions:  " << fmt->extensions << std::endl;
	}
}

void StreamParameters::showEncoderProperties(const char* codec_name)
{
	AVCodec* codec = avcodec_find_encoder_by_name(codec_name);
	if (!codec) {
		std::cout << "could not find codec: " << codec_name << std::endl;
		return;
	}
	else {
		std::cout << "Codec Name: " << codec_name << std::endl;
	}
	if (const char* media_type_string = av_get_media_type_string(codec->type))
		std::cout << "Media Type: " << media_type_string << std::endl;

	switch (codec->type) {
	case AVMEDIA_TYPE_VIDEO:
		if (!codec->pix_fmts) {
			std::cout << "No known supported pixel formats" << std::endl;
		}
		else {
			std::cout << "Supported Pixel Formats:" << std::endl;
			const AVPixelFormat* p;
			for (p = codec->pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
				if (const char* pix_fmt_string = av_get_pix_fmt_name(*p))
					std::cout << "\t" << pix_fmt_string << std::endl;
			}
		}
		break;
	case AVMEDIA_TYPE_AUDIO:
		if (!codec->sample_fmts) {
			std::cout << "No known supported sample formats" << std::endl;
		}
		else {
			std::cout << "Supported Sample Formats:" << std::endl;
			const AVSampleFormat* s;
			for (s = codec->sample_fmts; *s != AV_SAMPLE_FMT_NONE; s++) {
				if (const char* sample_fmt_string = av_get_sample_fmt_name(*s))
					std::cout << "    " << sample_fmt_string << std::endl;
			}
		}
		if (!codec->supported_samplerates) {
			std::cout << "No known supported sample rate" << std::endl;
		}
		else {
			std::cout << "Supported Sample Rates:" << std::endl;
			const int* r;
			for (r = codec->supported_samplerates; *r != 0; r++) {
				std::cout << "    " << *r << std::endl;
			}
		}
		if (!codec->channel_layouts) {
			std::cout << "No known supported channel layouts" << std::endl;
		}
		else {
			const uint64_t* c;
			for (c = codec->channel_layouts; *c != 0; c++) {
				int nb_channels = av_get_channel_layout_nb_channels(*c);
				std::cout << "    " << nb_channels << " channel layout" << std::endl;
				char buf[256] = { 0 };
				av_get_channel_layout_string(buf, 256, nb_channels, *c);
				std::cout << "        " << buf << std::endl;
 			}
		}
		break;
	default:
		std::cout << "Unknown media type" << std::endl;
		return;
	}
}

std::string StreamParameters::toString(AVMediaType media_type) const
{
	const char* pix_fmt_name = av_get_pix_fmt_name(pix_fmt);
	if (!pix_fmt_name) pix_fmt_name = "NULL";
	const char* sample_fmt_name = av_get_sample_fmt_name(sample_fmt);
	if (!sample_fmt_name) sample_fmt_name = "NULL";
	const char* audio_codec = audio_codec_name;
	if (!audio_codec) audio_codec = "NULL";
	const char* video_codec = video_codec_name;
	if (!video_codec) video_codec = "NULL";
	const char* fmt = format;
	if (!fmt) fmt = "NULL";

	std::string str;
	switch (media_type) {
	case AVMEDIA_TYPE_AUDIO:
		str = std::string("AUDIO PARAMS\n")
			+ "format:      " + std::string(fmt) + "\n"
			+ "sample_fmt:  " + std::string(sample_fmt_name) + "\n"
			+ "time_base:   " + std::to_string(audio_time_base.num) + " / " + std::to_string(audio_time_base.den) + "\n"
			+ "bit_rate:    " + std::to_string(audio_bit_rate) + "\n"
			+ "sample_rate: " + std::to_string(sample_rate) + "\n"
			+ "channels:    " + std::to_string(channels) + "\n"
			+ "nb_samples:  " + std::to_string(nb_samples) + "\n"
			+ "codec:       " + std::string(audio_codec) + "\n";
		break;
	case AVMEDIA_TYPE_VIDEO:
		str = std::string("VIDEO PARAMS\n")
			+ "format:     " + std::string(fmt) + "\n"
			+ "pix_fmt:    " + std::string(pix_fmt_name) + "\n"
			+ "width:      " + std::to_string(width) + "\n"
			+ "height:     " + std::to_string(height) + "\n"
			+ "time_base:  " + std::to_string(video_time_base.num) + " / " + std::to_string(video_time_base.den) + "\n"
			+ "bit_rate:   " + std::to_string(video_bit_rate) + "\n"
			+ "frame_rate: " + std::to_string(frame_rate) + "\n"
			+ "codec:      " + std::string(video_codec) + "\n";
		break;
	default:
		str = "UNKNOWN MEDIA TYPE";
	}

	return str;
}