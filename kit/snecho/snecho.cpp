#include "daisy_patch_sm.h"
#include "daisysp.h"

#include "models/snes.h"
#include "models/psx.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

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

    snes.cfg.echoLength   = hw.controls[CV_1].Value();
    snes.cfg.echoFeedback = hw.controls[CV_2].Value();
    snes.cfg.wetDry       = hw.controls[CV_3].Value();
    snes.cfg.filter       = hw.controls[CV_4].Value();
    if(button.RisingEdge())
    {
        snes.ClearBuffer();
        psx.ClearBuffer();
    }

    for(size_t i = 0; i < size; i++)
    {
        // TODO: crossfade on toggle
        if(toggle.Pressed())
        {
            psx.Process(IN_L[i], IN_R[i], OUT_L[i], OUT_R[i]);
        }
        else
        {
            snes.Process(IN_L[i], IN_R[i], OUT_L[i], OUT_R[i]);
        }
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(8); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);

    button.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
    toggle.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());

    hw.StartAudio(AudioCallback);
    while(1) {}
}
