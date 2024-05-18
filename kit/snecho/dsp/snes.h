#pragma once

namespace SNES
{
struct Config
{
    float echoLength;
    float echoFeedback;
    float filter;
};

struct Modulations
{
    float echoLength;
    float echoFeedback;
    float filter;
};
static constexpr int32_t kMaxEchoMs = 240;
// snap to 16ms increments, 16 * 32000 / 1000
static constexpr int32_t kEchoIncrementSamples = 512;

class Model
{
  public:
    Model(int32_t sampleRate, int16_t* echoBuffer, size_t echoBufferSize);
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
    int16_t* mEchoBuffer;
    size_t   mEchoBufferSize;
    size_t   mBufferIndex = 0;
    int16_t  FIRBuffer[8];
    // any coeffecients are allowed, but historically accurate coeffecients can be found here:
    // https://sneslab.net/wiki/FIR_Filter#Examples
    int16_t FIRCoeff[8] = {0x58, 0xBF, 0xDB, 0xF0, 0xFE, 0x07, 0x0C, 0x0C};
};
} // namespace SNES