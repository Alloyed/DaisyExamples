#include "daisy_patch_sm.h"

#include "dsp/snes.h"
#include "dsp/psx.h"
#include "dsp/util.h"
#include "dsp/resampler.h"

using namespace daisy;
using namespace patch_sm;

#define SAMPLE_RATE 96000

DaisyPatchSM hw;
Switch       button, toggle;

constexpr size_t snesBufferSize
    = SNES::Model::GetBufferDesiredSizeInt16s(SNES::kOriginalSampleRate);
int16_t     snesBuffer[snesBufferSize];
SNES::Model snes(SNES::kOriginalSampleRate, snesBuffer, snesBufferSize);
Resampler   snesSampler(SNES::kOriginalSampleRate, SAMPLE_RATE);

constexpr size_t psxBufferSize
    = PSX::Model::GetBufferDesiredSizeFloats(PSX::kOriginalSampleRate);
float      psxBuffer[psxBufferSize];
PSX::Model psx(PSX::kOriginalSampleRate, psxBuffer, psxBufferSize);
Resampler  psxSampler(PSX::kOriginalSampleRate, SAMPLE_RATE);

// 1.0f == psx, 0.0f == SNES
float snesToPsxFade = 0.0f;

// 10ms fade
static constexpr float fadeRate = (10.0f / 1000.0f) / SAMPLE_RATE;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    button.Debounce();
    toggle.Debounce();

    snes.cfg.echoLength   = hw.controls[CV_1].Value();
    snes.mod.echoLength   = hw.controls[CV_5].Value();
    snes.cfg.echoFeedback = hw.controls[CV_2].Value();
    snes.mod.echoFeedback = hw.controls[CV_6].Value();
    snes.cfg.filter       = hw.controls[CV_3].Value();
    snes.mod.filter       = hw.controls[CV_7].Value();

    // PSX has no parameters yet D:

    float wetDry = clampf(
        hw.controls[CV_4].Value() + hw.controls[CV_8].Value(), 0.0f, 1.0f);

    if(button.RisingEdge() || hw.gate_in_1.Trig())
    {
        snes.ClearBuffer();
        psx.ClearBuffer();
    }

    for(size_t i = 0; i < size; i++)
    {
        // fade should take about 10ms
        if(toggle.Pressed() != hw.gate_in_2.State())
        {
            // fade to PSX
            snesToPsxFade = clampf(snesToPsxFade + fadeRate, 0.0f, 1.0f);
        }
        else
        {
            // fade to SNES
            snesToPsxFade = clampf(snesToPsxFade - fadeRate, 0.0f, 1.0f);
        }

        float snesLeft, snesRight, psxLeft, psxRight;
        psxSampler.Process(
            IN_L[i],
            IN_R[i],
            psxLeft,
            psxRight,
            [](float inLeft, float inRight, float &outLeft, float &outRight)
            { psx.Process(inLeft, inRight, outLeft, outRight); });

        snesSampler.Process(
            IN_L[i],
            IN_R[i],
            snesLeft,
            snesRight,
            [](float inLeft, float inRight, float &outLeft, float &outRight)
            { snes.Process(inLeft, inRight, outLeft, outRight); });

        float left  = fadeCpowf(snesLeft, psxLeft, snesToPsxFade);
        float right = fadeCpowf(snesRight, psxRight, snesToPsxFade);

        OUT_L[i] = fadeCpowf(IN_L[i], left, wetDry);
        OUT_R[i] = fadeCpowf(IN_R[i], right, wetDry);
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(2); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);

    button.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
    toggle.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());

    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.SetLed(toggle.Pressed());
        System::Delay(1);
    }
}
