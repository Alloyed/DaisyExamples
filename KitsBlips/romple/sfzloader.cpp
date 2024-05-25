#include "sfzloader.h"

#include "fatfs.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>

inline void ltrim(std::string &s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(),
                         s.end(),
                         [](unsigned char ch) { return !std::isspace(ch); }));
}

inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(),
                         s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

inline void trim(std::string &s)
{
    size_t comment = s.find("//");
    if(comment != std::string::npos)
    {
        s.erase(comment);
    }
    ltrim(s);
    rtrim(s);
}

namespace romple
{
enum State
{
    NONE,
    REGION,
    OPCODE,
    STRING,
};
SFZLoader::Result SFZLoader::LoadSync(const char *filename, SfzData &outSfzData)
{
    // breaking my usual rule around avoiding allocations here out of laziness
    FIL  fileStorage;
    FIL *pFile = &fileStorage;
    pOut       = &outSfzData;

    if(f_open(pFile, filename, FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        return Result::ERR_GENERIC;
    }

    // read line-by-line
    char lineBuffer[512];
    while(!f_eof(pFile))
    {
        const char *ok = f_gets(lineBuffer, 512, pFile);
        if(ok != lineBuffer)
        {
            return Result::ERR_GENERIC;
        }
        std::string line = lineBuffer;
        trim(line);
        size_t pos = 0;
        while(pos < line.size())
        {
            if(line[pos] == '<')
            {
                // header
                size_t headerEnd = line.find('>', pos);
                if(headerEnd == std::string::npos)
                {
                    return Result::ERR_GENERIC;
                }
                std::string headerName = line.substr(pos + 1, headerEnd - 1);
                PushHeader(headerName);
                pos = headerEnd + 1;
            }
            else if(std::isalpha(line[pos]))
            {
                // opcode
                size_t opcodeEnd = line.find('=', pos);
                if(opcodeEnd == std::string::npos)
                {
                    return Result::ERR_GENERIC;
                }
                std::string opcode = line.substr(pos, opcodeEnd - 1);
                pos                = opcodeEnd + 1;
                // not spec compliant, in fact we are supposed to tokenize by whitespace and any token
                // (excluding '=' and not starting with '<') should be added to the param
                size_t paramEnd = line.find(' ', pos);
                if(paramEnd == std::string::npos)
                {
                    paramEnd = line.size();
                }
                std::string param = line.substr(pos, paramEnd);
                PushOpcode(opcode, param);
                pos = paramEnd;
            }
            else
            {
                // skip
                pos++;
            }
        }
    }

    return Result::OK;
}

void SFZLoader::PushHeader(const std::string &headerName)
{
    // https://sfzformat.com/headers/
    if(headerName == "region")
    {
        SfzRegion copiedRegion = mGroup;
        pOut->regions.push_back(copiedRegion);
    }
    else if(headerName == "group")
    {
        mGroup = {};
    }
    // ignored
    else
    {
        //printf("unhandled header: %s\n", headerName.c_str())
    }
}

void SFZLoader::PushOpcode(const std::string &opcode, const std::string &param)
{
    // https://sfzformat.com/opcodes/
    // Sample playback
    if(opcode == "sample")
    {
        Region().fileName = param;
    }
    else if(opcode == "offset")
    {
        Region().startIndex = std::strtol(param.c_str(), nullptr, 0);
    }
    else if(opcode == "end")
    {
        Region().endIndex = std::strtol(param.c_str(), nullptr, 0);
    }
    // key mapping
    else if(opcode == "key")
    {
        int16_t value           = std::strtol(param.c_str(), nullptr, 0);
        Region().lowKey         = value;
        Region().hiKey          = value;
        Region().pitchKeyCenter = value;
    }
    else if(opcode == "lokey")
    {
        int16_t value   = std::strtol(param.c_str(), nullptr, 0);
        Region().lowKey = value;
    }
    else if(opcode == "hikey")
    {
        int16_t value  = std::strtol(param.c_str(), nullptr, 0);
        Region().hiKey = value;
    }
    else if(opcode == "pitch_keycenter")
    {
        int16_t value           = std::strtol(param.c_str(), nullptr, 0);
        Region().pitchKeyCenter = value;
    }
    // ignore unhandled opcodes
    else
    {
        // printf("Unhandled opcode: %s\n", opcode.c_str())
    }
}
} // namespace romple