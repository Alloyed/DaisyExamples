#include "dsp/snes.h"
#include "dsp/util.h"

#include <climits>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>

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


int16_t SNES::Model::ProcessFIR(int16_t inSample)
{
    // update FIR buffer
    for(size_t i = 0; i < 6; ++i)
    {
        mFIRBuffer[i] = mFIRBuffer[i + 1];
    }
    mFIRBuffer[7] = inSample;

    // update FIR coeffs

    // apply first 7 taps
    int16_t S = (mFIRCoeff[0] * mFIRBuffer[0] >> 6)
                + (mFIRCoeff[1] * mFIRBuffer[1] >> 6)
                + (mFIRCoeff[2] * mFIRBuffer[2] >> 6)
                + (mFIRCoeff[3] * mFIRBuffer[3] >> 6)
                + (mFIRCoeff[4] * mFIRBuffer[4] >> 6)
                + (mFIRCoeff[5] * mFIRBuffer[5] >> 6)
                + (mFIRCoeff[6] * mFIRBuffer[6] >> 6);
    // Clip
    S = S & 0xFFFF;
    // Apply last tap
    S = S + (mFIRCoeff[7] * mFIRBuffer[7] >> 6);

    // Clamp
    S = S > 32767 ? 32767 : S;
    S = S < -32768 ? -32768 : S;
    return S;
}

SNES::Model::Model(int32_t  _sampleRate,
                   int16_t* _echoBuffer,
                   size_t   _echoBufferSize)
: mEchoBuffer(_echoBuffer), mEchoBufferSize(_echoBufferSize)
{
    //assert(_sampleRate == 32000); // TODO: other sample rates
    //assert(_echoBufferSize == GetBufferDesiredSizeInt16s(_sampleRate));
    ClearBuffer();
}

void SNES::Model::ClearBuffer()
{
    memset(mFIRBuffer, 0, kFIRTaps * sizeof(mFIRBuffer[0]));
    memset(mEchoBuffer, 0, mEchoBufferSize * sizeof(mEchoBuffer[0]));
}


void SNES::Model::Process(float  inputLeft,
                          float  inputRight,
                          float& outputLeft,
                          float& outputRight)
{
    float delay       = clampf(cfg.echoLength + mod.echoLength, 0.0f, 1.0f);
    float feedback    = clampf(cfg.echoFeedback + mod.echoFeedback, 0.0f, 1.0f);
    float firResponse = cfg.filter + mod.filter;

    mBufferIndex = (mBufferIndex + 1) % mEchoBufferSize;

    // summing mixdown. if right is normalled to left, acts as a mono signal.
    float   inputFloat = (inputLeft + inputRight) * 0.5f;
    int16_t inputNorm  = static_cast<int16_t>(inputFloat * INT16_MAX);

    // TODO: hysteresis
    size_t delayNumSamples = static_cast<size_t>(
        roundTof(delay * mEchoBufferSize, kEchoIncrementSamples));

    int16_t delayedSample
        = mEchoBuffer[(mBufferIndex - delayNumSamples) % mEchoBufferSize];
    int16_t filteredSample = ProcessFIR(delayedSample);

    // store current state in echo buffer /w feedback
    mEchoBuffer[mBufferIndex]
        = inputNorm
          + static_cast<int16_t>(static_cast<float>(filteredSample) * feedback);

    float echoFloat = static_cast<float>(filteredSample) / INT16_MAX;

    // The real SNES let you pick between inverting the right channel and not doing that.
    // if you don't want it here, just use a mult on the left output ;)
    outputLeft  = echoFloat;
    outputRight = echoFloat * -1.0f;
}
