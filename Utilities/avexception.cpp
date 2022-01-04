#include "avexception.h"

AVException::AVException(const std::string& msg)
{
    buffer = msg.c_str();
}

void AVExceptionHandler::ck(int ret)
{
    if (ret < 0) throw AVException("an AV exception has occurred");
}

void AVExceptionHandler::ck(int ret, CmdTag cmd_tag)
{
    if (ret < 0) {
        char av_str[256];
        av_strerror(ret, av_str, 256);
        throw AVException(tag(cmd_tag) + std::string(" has failed with error: ") + av_str);
    }
}

void AVExceptionHandler::ck(int ret, const std::string& msg)
{
    if (ret < 0) {
        char av_str[256];
        av_strerror(ret, av_str, 256);
        throw AVException(msg + std::string(" ") + av_str);
    }
}

void AVExceptionHandler::ck(void* arg, CmdTag cmd_tag)
{
    if (arg == NULL) throw getNullException(cmd_tag);
}

void AVExceptionHandler::ck(void* arg, const std::string& msg)
{
    if (arg == NULL) throw AVException(msg);
}

void AVExceptionHandler::ck(const void* arg, CmdTag cmd_tag)
{
    if (arg == NULL) throw getNullException(cmd_tag);
}

void AVExceptionHandler::ck(const void* arg, const std::string& msg)
{
    if (arg == NULL) throw AVException(msg);
}

const AVException AVExceptionHandler::getNullException(CmdTag cmd_tag)
{
    if (cmd_tag == CmdTag::NONE) {
        return AVException("a NULL exception has occurred");
    }
    else {
        return AVException(tag(cmd_tag) + std::string(" has failed with NULL value"));
    }
}

void AVExceptionHandler::showError(const std::string& error_msg)
{
    std::cout << error_msg << std::endl;
}

const char* AVExceptionHandler::tag(CmdTag cmd_tag)
{
    switch (cmd_tag) {
    case CmdTag::AO2:
        return "avcodec_open2";
    case CmdTag::AOI:
        return "avformat_open_input";
    case CmdTag::ACI:
        return "avformat_close_input";
    case CmdTag::AFSI:
        return "avformat_find_stream_info";
    case CmdTag::AFBS:
        return "av_find_best_stream";
    case CmdTag::APTC:
        return "avcodec_parameters_to_context";
    case CmdTag::APFC:
        return "avcodec_parameters_from_context";
    case CmdTag::AWH:
        return "av_write_header";
    case CmdTag::AWT:
        return "av_write_trailer";
    case CmdTag::AO:
        return "avio_open";
    case CmdTag::AC:
        return "avio_close";
    case CmdTag::ACP:
        return "avio_closep";
    case CmdTag::AAOC2:
        return "avformat_alloc_output_context2";
    case CmdTag::AFMW:
        return "av_frame_make_writable";
    case CmdTag::AFGB:
        return "av_frame_get_buffer";
    case CmdTag::AHCC:
        return "av_hwdevice_ctx_create";
    case CmdTag::AWF:
        return "av_write_frame";
    case CmdTag::ASP:
        return "avcodec_send_packet";
    case CmdTag::ASF:
        return "av_seek_frame";
    case CmdTag::AEV2:
        return "avcodec_encode_video2";
    case CmdTag::ARF:
        return "av_read_frame";
    case CmdTag::ADV2:
        return "av_decode_video2";
    case CmdTag::ARP:
        return "avcodec_recieve_packet";
    case CmdTag::AIWF:
        return "av_interleaved_write_frame";
    case CmdTag::AFE:
        return "avcodec_find_encoder";
    case CmdTag::AFD:
        return "avcodec_find_decoder";
    case CmdTag::AAC3:
        return "avcodec_alloc_context3";
    case CmdTag::AFA:
        return "av_frame_alloc";
    case CmdTag::AAC:
        return "avformat_alloc_context";
    case CmdTag::AFC:
        return "av_frame_copy";
    case CmdTag::ABR:
        return "av_buffer_ref";
    case CmdTag::AGF:
        return "av_guess_format";
    case CmdTag::AHCA:
        return "avcodec_find_encoder_by_name";
    case CmdTag::AHCI:
        return "av_hwframe_ctx_init";
    case CmdTag::AHGB:
        return "av_hwframe_get_buffer";
    case CmdTag::AFEBN:
        return "avcodec_find_encoder_by_name";
    case CmdTag::AICTB:
        return "av_image_copy_to_buffer";
    case CmdTag::APFDG:
        return "av_pix_fmt_desc_get";
    case CmdTag::AGPFN:
        return "av_get_pix_fmt_name";
    case CmdTag::AHFTBN:
        return "av_hwdevice_find_type_by_name";
    case CmdTag::AGHC:
        return "avcodec_get_hw_config";
    case CmdTag::AHTD:
        return "av_hwframe_transfer_data";
    case CmdTag::ANS:
        return "avformat_new_stream";
    case CmdTag::AFR:
        return "av_frame_ref";
    case CmdTag::AFCP:
        return "av_frame_copy_props";
    case CmdTag::SGC:
        return "sws_getContext";
    case CmdTag::AFIF:
        return "av_find_input_format";
    case CmdTag::APA:
        return "av_packet_alloc";
    case CmdTag::ADC:
        return "av_dict_copy";
    case CmdTag::AIA:
        return "av_image_alloc";
    case CmdTag::AM:
        return "av_malloc";
    case CmdTag::SA:
        return "swr_alloc";
    case CmdTag::SI:
        return "swr_init";
    case CmdTag::SC:
        return "swr_convert";
    default:
        return "";
    }
}