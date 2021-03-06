#ifndef EXCEPTION_H
#define EXCEPTION_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
}

#include <exception>
#include <iostream>
#include <functional>
#include <sstream>


    enum CmdTag {
        NONE,
        AO2,
        AOI,
        ACI,
        AFSI,
        APTC,
        APFC,
        AWH,
        AWT,
        AO,
        AC,
        ACP,
        AAOC2,
        AFMW,
        AFGB,
        AHCC,
        AFBS,
        AWF,
        ASP,
        ASF,
        AEV2,
        ARF,
        ADV2,
        ARP,
        AIWF,
        AFE,
        AFD,
        AAC3,
        AFA,
        AAC,
        AFC,
        ABR,
        AGF,
        AGA,
        AGC,
        AGPP,
        AGCF,
        AHCA,
        AHCI,
        AHGB,
        AFEBN,
        AICTB,
        AGPFN,
        ABAFF,
        APFDG,
        AHFTBN,
        AOSIL,
        AFDBN,
        AGHC,
        AHTD,
        ANS,
        AFCP,
        SGC,
        AFIF,
        APA,
        ADC,
        AIA,
        AFR,
        AM,
        SA,
        SI,
        SC,
        SS
    };

    enum MsgPriority {
        CRITICAL,
        DEBUG,
        INFO
    };

namespace av
 {
        class Exception : public std::exception
    {
    public:
        Exception(const char* msg);
        Exception(const std::string& msg);

        const char* what() const throw () {
            return buffer;
        }

        const char* buffer;
    };

    class ExceptionHandler
    {

    public:
        void ck(int ret);
        void ck(int ret, CmdTag cmd_tag);
        void ck(int ret, const std::string& msg);
        void ck(void* arg, CmdTag cmd_tag = CmdTag::NONE);
        //void ck(void* arg, const std::string& msg);
        //void ck(const void* arg, CmdTag cmd_tag = CmdTag::NONE);
        //void ck(const void* arg, const std::string& msg);

        const Exception getNullException(CmdTag cmd_tag);
        const char* tag(CmdTag cmd_tag);

        std::function<void(const std::string&, MsgPriority, const std::string&)> fnMsgOut = nullptr;
        void msg(const std::string& msg, MsgPriority priority = MsgPriority::INFO, const std::string& qualifier = "");
    };

}


#endif // EXCEPTION_H
