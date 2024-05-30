#pragma once

namespace SNES
{
struct Config
{
    // [0, 1]: rounded up to nearest 16 ms, up to kMaxEchoMs
    float echoBufferSize;
    // [-1, 1]: moves the read head up/down up to 16 ms. can be used for chorus-y effects. (extension, not available on original hardware)
    float echoDelayMod;
    // [0, 1]: feedback. 0 is no feedback, 1 is 100% (watch out!)
    float echoFeedback;
    // [0, 1]: FIR filter setting. 0-1 transitions smoothly between common FIR filter settings used in SNES games.
    float filter;
};

struct Modulations
{
    // added to the equivalent config variables
    float echoBufferSize;
    float echoDelayMod;
    float echoFeedback;
    float filter;
};

// low sample rate for OG crunchiness
static constexpr int32_t kOriginalSampleRate = 32000;
// max echo depth
static constexpr int32_t kMaxEchoMs = 240;
// snap to 16ms increments, 16 * 32000 / 1000
static constexpr int32_t kEchoIncrementSamples = 512;
// hardcoded into the snes. not sure how sample rate affects this
static constexpr size_t kFIRTaps = 8;

class Model
{
  public:
    Model(int32_t sampleRate, int16_t* echoBuffer, size_t echoBufferCapacity);
    void Process(float  inputLeft,
                 float  inputRight,
                 float& outputLeft,
                 float& outputRight);

    static constexpr size_t GetBufferDesiredSizeInt16s(int32_t sampleRate)
    {
        return kMaxEchoMs * (sampleRate / 1000);
    }

    void ClearBuffer();

    Config      cfg;
    Modulations mod;

  private:
    int16_t ProcessFIR(int16_t in);

    int16_t* mEchoBuffer;
    size_t   mEchoBufferCapacity;
    size_t   mEchoBufferSize;
    size_t   mBufferIndex = 0;
    int16_t  mFIRBuffer[kFIRTaps];
    // any coeffecients are allowed, but historically accurate coeffecients can be found here:
    // https://sneslab.net/wiki/FIR_Filter#Examples
    int16_t mFIRCoeff[kFIRTaps]
        = {0x58, 0xBF, 0xDB, 0xF0, 0xFE, 0x07, 0x0C, 0x0C};
};
} // namespace SNES