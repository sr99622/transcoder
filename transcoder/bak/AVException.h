#ifndef AVEXCEPTION_H
#define AVEXCEPTION_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <exception>
#include <iostream>

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
    AHCA,
    AHCI,
    AHGB,
    AFEBN,
    AICTB,
    AGPFN,
    APFDG,
    AHFTBN,
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
    SC
};

enum MsgPriority {
    CRITICAL,
    DEBUG,
    INFO
};

class AVException : public std::exception
{
public:
    AVException(const std::string& msg);

    const char* what() const throw () {
        return buffer;
    }

    const char* buffer;
};

class AVExceptionHandler
{

public:
    void ck(int ret);
    void ck(int ret, CmdTag cmd_tag);
    void ck(int ret, const std::string& msg);
    void ck(void* arg, CmdTag cmd_tag = CmdTag::NONE);
    void ck(void* arg, const std::string& msg);
    void ck(const void* arg, CmdTag cmd_tag = CmdTag::NONE);
    void ck(const void* arg, const std::string& msg);

    const AVException getNullException(CmdTag cmd_tag);
    const char* tag(CmdTag cmd_tag);
    void msg(const std::string& msg, MsgPriority priority = MsgPriority::INFO, const std::string& qualifier = "");
};

#endif // AVEXCEPTION_H
