#include "dsp/snes.h"

#include <climits>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>

constexpr size_t FIR_TAPS
    = 8; // hardcoded into the snes. not sure how sample rate affects this

/*
 * This is a rough transcription of the rules that govern the SNES's reverb/echo pathway, described here:
 * https://sneslab.net/wiki/FIR_Filter
 * I found a reddit comment claiming that the snes chip and the PSX chip are the same, but this seems to severly contradict that:
 * https://psx-spx.consoledev.net/soundprocessingunitspu/#spu-reverb-formula
 */

/*
https://problemkaputt.de/fullsnes.htm#snesaudioprocessingunitapu
DSP Mixer/Reverb Block Diagram (c=channel, L/R)
  c0 --->| ADD    |                      |MVOLc| Master Volume   |     |
  c1 --->| Output |--------------------->| MUL |---------------->|     |
  c2 --->| Mixing |                      |_____|                 |     |
  c3 --->|        |                       _____                  | ADD |--> c
  c4 --->|        |                      |EVOLc| Echo Volume     |     |
  c5 --->|        |   Feedback   .------>| MUL |---------------->|     |
  c6 --->|        |   Volume     |       |_____|                 |_____|
  c7 --->|________|    _____     |                      _________________
                      | EFB |    |                     |                 |
     EON  ________    | MUL |<---+---------------------|   Add FIR Sum   |
  c0 -:->|        |   |_____|                          |_________________|
  c1 -:->|        |      |                              _|_|_|_|_|_|_|_|_
  c2 -:->|        |      |                             |   MUL FIR7..0   |
  c3 -:->|        |      |         ESA=Addr, EDL=Len   |_7_6_5_4_3_2_1_0_|
  c4 -:->| ADD    |    __V__  FLG   _______________     _|_|_|_|_|_|_|_|_
  c5 -:->| Echo   |   |     | ECEN | Echo Buffer c |   | FIR Buffer c    |
  c6 -:->| Mixing |-->| ADD |--:-->|   RAM         |-->| (Hardware regs) |
  c7 -:->|________|   |_____|      |_______________|   |_________________|
                                   Newest --> Oldest    Newest --> Oldest
*/


int16_t FIRBuffer[FIR_TAPS];

// any coeffecients are allowed, but historically accurate coeffecients can be found here:
// https://sneslab.net/wiki/FIR_Filter#Examples
int16_t FIRCoeff[] = {0x58, 0xBF, 0xDB, 0xF0, 0xFE, 0x07, 0x0C, 0x0C};

int16_t ProcessFIR(int16_t inSample)
{
    // update FIR buffer
    for(size_t i = 0; i < 6; ++i)
    {
        FIRBuffer[i] = FIRBuffer[i + 1];
    }
    FIRBuffer[7] = inSample;

    // update FIR coeffs

    // apply first 7 taps
    int16_t S = (FIRCoeff[0] * FIRBuffer[0] >> 6)
                + (FIRCoeff[1] * FIRBuffer[1] >> 6)
                + (FIRCoeff[2] * FIRBuffer[2] >> 6)
                + (FIRCoeff[3] * FIRBuffer[3] >> 6)
                + (FIRCoeff[4] * FIRBuffer[4] >> 6)
                + (FIRCoeff[5] * FIRBuffer[5] >> 6)
                + (FIRCoeff[6] * FIRBuffer[6] >> 6);
    // Clip
    S = S & 0xFFFF;
    // Apply last tap
    S = S + (FIRCoeff[7] * FIRBuffer[7] >> 6);

    // Clamp
    S = S > 32767 ? 32767 : S;
    S = S < -32768 ? -32768 : S;
    return S;
}

SNES::Model::Model(int32_t  _sampleRate,
                   int16_t* _echoBuffer,
                   size_t   _echoBufferSize)
: echoBuffer(_echoBuffer), echoBufferSize(_echoBufferSize)
{
    //assert(_sampleRate == 32000); // TODO: other sample rates
    //assert(_echoBufferSize == GetBufferDesiredSizeInt16s(_sampleRate));
    ClearBuffer();
}

void SNES::Model::ClearBuffer()
{
    memset(FIRBuffer, 0, FIR_TAPS * sizeof(FIRBuffer[0]));
    memset(echoBuffer, 0, echoBufferSize * sizeof(echoBuffer[0]));
}


void SNES::Model::Process(float  inputLeft,
                          float  inputRight,
                          float& outputLeft,
                          float& outputRight)
{
    float delay
        = std::max(0.0f, std::min(1.0f, cfg.echoLength + mod.echoLength));
    float feedback
        = std::max(0.0f, std::min(0.9f, cfg.echoFeedback + mod.echoFeedback));
    float wet         = std::max(0.0f, std::min(1.0f, cfg.wetDry + mod.wetDry));
    float firResponse = cfg.filter + mod.filter;

    bufferIndex = (bufferIndex + 1) % echoBufferSize;

    (void)inputRight;
    float   inputFloat = inputLeft;
    int16_t inputNorm  = static_cast<int16_t>(inputFloat * INT16_MAX);

    // pull delayed sample
    size_t delayNumSamples
        = static_cast<size_t>(delay * echoBufferSize) % echoBufferSize;
    int16_t delayedSample
        = echoBuffer[(bufferIndex - delayNumSamples) % echoBufferSize];
    int16_t filteredSample = ProcessFIR(delayedSample);

    // store current state in echo buffer /w feedback
    echoBuffer[bufferIndex]
        = inputNorm
          + static_cast<int16_t>(static_cast<float>(filteredSample) * feedback);

    float echoFloat   = static_cast<float>(filteredSample) / INT16_MAX;
    float outputFloat = (inputFloat * (1.0f - wet)) + (echoFloat * wet);

    outputLeft  = outputFloat;
    outputRight = outputFloat * -1.0f;
}
