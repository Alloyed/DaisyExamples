#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;


struct Config
{
    float echoLength;
    float echoFeedback;
    float filter;
    float wetDry;
};

struct Modulations
{
    float echoLength;
    float echoFeedback;
    float filter;
    float wetDry;
};

DaisyPatchSM hw;
Config       cfg;
Modulations  mod;

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


int16_t FIRBuffer[8];

// any coeffecients are allowed, but historically accurate coeffecients can be found here:
// https://sneslab.net/wiki/FIR_Filter#Examples
int16_t FIRCoeff[8] = {0x58, 0xBF, 0xDB, 0xF0, 0xFE, 0x07, 0x0C, 0x0C};

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

// per dev manual
// Delay time o is an interval of 16 msec, and is variable within a range of 0 ~ 240 msec.
#define MAX_ECHO_MS 240
#define SAMPLES_PER_MS 32 // 32khz / 1000
int16_t echoBuffer[MAX_ECHO_MS * SAMPLES_PER_MS];
size_t  bufferIndex = 0;

float ProcessSnecho(float inputFloat)
{
    float delay       = cfg.echoLength + mod.echoLength;
    float feedback    = cfg.echoFeedback + mod.echoFeedback;
    float wet         = cfg.wetDry + mod.wetDry;
    float firResponse = cfg.filter + mod.filter;

    bufferIndex = (bufferIndex + 1) % sizeof(echoBuffer);

    int16_t inputNorm = static_cast<int16_t>(inputFloat * INT16_MAX);

    // pull delayed sample
    size_t delayNumSamples = std::clamp(
        static_cast<size_t>(delay / 16.0f) * 16u, 0u, sizeof(echoBuffer));
    int16_t delayedSample
        = echoBuffer[(bufferIndex - delayNumSamples) % sizeof(echoBuffer)];
    int16_t filteredSample = ProcessFIR(delayedSample);

    // store current state in echo buffer /w feedback
    echoBuffer[bufferIndex]
        = inputNorm
          + static_cast<int16_t>(static_cast<float>(filteredSample) * feedback);

    float echoFloat   = static_cast<float>(filteredSample) / INT16_MAX;
    float outputFloat = inputFloat * (1.0f - wet) + echoFloat * wet;
    return outputFloat;
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    for(size_t i = 0; i < size; i++)
    {
        float outSample = ProcessSnecho(IN_L[i] + IN_R[i]);
        OUT_L[i]        = outSample;
        // SNES supported "stereo surround" by inverting phase at the very end
        // of the chain. this is emulated here, but you can just use the left
        // output on its own if this isn't desired.
        OUT_R[i] = outSample * -1.0f;
    }
}

int main(void)
{
    memset(FIRBuffer, 0, sizeof(FIRBuffer) * sizeof(FIRBuffer[0]));
    memset(echoBuffer, 0, sizeof(echoBuffer) * sizeof(echoBuffer[0]));

    hw.Init();
    hw.SetAudioBlockSize(8); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);
    hw.StartAudio(AudioCallback);
    while(1) {}
}
