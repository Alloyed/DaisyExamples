#include "dsp/snes.h"
#include "dsp/util.h"

#include <climits>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>

/*
  This is a rough transcription of the rules that govern the SNES's reverb/echo pathway, references below:

Thank you especially msx @heckscaper for providing extra info/examples
https://sneslab.net/wiki/FIR_Filter
https://problemkaputt.de/fullsnes.htm#snesaudioprocessingunitapu
https://wiki.superfamicom.org/spc700-reference#dsp-echo-function-1511      
https://www.youtube.com/watch?v=JC0PywZvKeg

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
                   size_t   _echoBufferCapacity)
: mEchoBuffer(_echoBuffer), mEchoBufferCapacity(_echoBufferCapacity)
{
    //assert(_sampleRate == kOriginalSampleRate); // TODO: other sample rates
    //assert(_echoBufferCapacity == GetBufferDesiredSizeInt16s(_sampleRate));
    ClearBuffer();
}

void SNES::Model::ClearBuffer()
{
    memset(mFIRBuffer, 0, kFIRTaps * sizeof(mFIRBuffer[0]));
    memset(mEchoBuffer, 0, mEchoBufferCapacity * sizeof(mEchoBuffer[0]));
}

void SNES::Model::Process(float  inputLeft,
                          float  inputRight,
                          float& outputLeft,
                          float& outputRight)
{
    float targetSize
        = clampf(cfg.echoBufferSize + mod.echoBufferSize, 0.0f, 1.0f);
    float delayMod    = clampf(cfg.echoDelayMod + mod.echoDelayMod, 0.0f, 1.0f);
    float feedback    = clampf(cfg.echoFeedback + mod.echoFeedback, 0.0f, 1.0f);
    float firResponse = clampf(cfg.filter + mod.filter, 0.0f, 1.0f);

    // TODO: hysteresis
    size_t targetSizeSamples = static_cast<size_t>(
        roundTof(targetSize * mEchoBufferCapacity, kEchoIncrementSamples));
    mBufferIndex = (mBufferIndex + 1) % mEchoBufferSize;

    if(targetSizeSamples != mEchoBufferSize && mBufferIndex == 0)
    {
        // only resize buffer at the end of of the last delay
        // based on this line in docs
        // > *** This is because the echo hardware doesn't actually read the buffer designation values until it reaches the END of the old buffer!
        mEchoBufferSize = targetSizeSamples;
    }

    // summing mixdown. if right is normalled to left, acts as a mono signal.
    float   inputFloat = (inputLeft + inputRight) * 0.5f;
    int16_t inputNorm  = static_cast<int16_t>(inputFloat * INT16_MAX);

    size_t delayModSamples = static_cast<size_t>(delayMod * mEchoBufferSize);

    int16_t delayedSample
        = mEchoBuffer[(mBufferIndex - delayModSamples) % mEchoBufferSize];
    //int16_t filteredSample = ProcessFIR(delayedSample);
    int16_t filteredSample = delayedSample;

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
