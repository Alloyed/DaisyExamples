#pragma once

#include <vector>
#include <string>

namespace romple
{

//https://sfzformat.com/modulations/envelope_generators/#sfz-1-eg-opcodes
struct SfzEnv
{
    float start   = 0.0f;
    float delay   = 0.0f;
    float attack  = 0.0f;
    float hold    = 0.0f;
    float decay   = 0.0f;
    float sustain = 1.0f;
    float release = 0.001f;
};

struct SfzRegion
{
    // the midi note the sample is centered at. when this note is played, the sample will not have any pitch shift applied.
    int16_t pitchKeyCenter = 60;
    // the lowest midi note that will trigger the region
    int16_t lowKey = 0;
    // the highest midi note that will trigger the region
    int16_t hiKey = 127;

    SfzEnv vcaEnvelope;

    // the index within the sample to start after trigger
    size_t startIndex = 0;
    // the index within the sample to end at
    size_t      endIndex = 0;
    std::string fileName = "";
};

struct SfzData
{
    std::vector<SfzRegion> regions;
};

class SFZLoader
{
  public:
    enum class Result
    {
        OK,
        ERR_GENERIC
    };
    SFZLoader() {}

    Result LoadSync(const char *filename, SfzData &outSfzData);

  private:
    void PushHeader(const std::string &headerName);
    void PushOpcode(const std::string &opcode, const std::string &parameter);
    SfzRegion &Region() { return pOut->regions.back(); }

    SfzData  *pOut;
    SfzRegion mGroup;
};

} // namespace romple