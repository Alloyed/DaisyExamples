#include "daisy_patch_sm.h"

#include "dsp/snes.h"
#include "dsp/psx.h"
#include "dsp/util.h"

using namespace daisy;
using namespace patch_sm;

#define SAMPLE_RATE 32000

DaisyPatchSM hw;
Switch       button, toggle;

constexpr size_t snesBufferSize
    = SNES::Model::GetBufferDesiredSizeInt16s(SAMPLE_RATE);
int16_t     snesBuffer[snesBufferSize];
SNES::Model snes(SAMPLE_RATE, snesBuffer, snesBufferSize);

constexpr size_t psxBufferSize
    = PSX::Model::GetBufferDesiredSizeFloats(SAMPLE_RATE);
float      psxBuffer[psxBufferSize];
PSX::Model psx(SAMPLE_RATE, psxBuffer, psxBufferSize);

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    button.Debounce();
    toggle.Debounce();

    // pot input and CV input are summed here (there isn't enough space on the panel for attenuators, unfortunately)
    snes.cfg.echoLength = clampf(
        hw.controls[CV_1].Value() + hw.controls[CV_5].Value(), 0.0f, 1.0f);
    snes.cfg.echoFeedback = clampf(
        hw.controls[CV_2].Value() + hw.controls[CV_6].Value(), 0.0f, 1.0f);
    snes.cfg.filter = clampf(
        hw.controls[CV_3].Value() + hw.controls[CV_7].Value(), 0.0f, 1.0f);
    float wetDry = clampf(
        hw.controls[CV_4].Value() + hw.controls[CV_8].Value(), 0.0f, 1.0f);

    if(button.RisingEdge() || hw.gate_in_1.Trig())
    {
        snes.ClearBuffer();
        psx.ClearBuffer();
    }

    for(size_t i = 0; i < size; i++)
    {
        float left;
        float right;
        // TODO: crossfade on toggle
        if(toggle.Pressed())
        {
            psx.Process(IN_L[i], IN_R[i], left, right);
        }
        else
        {
            snes.Process(IN_L[i], IN_R[i], left, right);
        }
        OUT_L[i] = fadeCpowf(IN_L[i], left, wetDry);
        OUT_R[i] = fadeCpowf(IN_R[i], right, wetDry);
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(2); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);

    button.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
    toggle.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());

    hw.StartAudio(AudioCallback);
    while(1) {}
}
