#include "daisy_patch_sm.h"

#include "dsp/dsp.h"
#include "dsp/voice.h"

using namespace daisy;
using namespace patch_sm;

DaisyPatchSM hw;

plaits::Modulations modulations;
plaits::Patch       patch;
plaits::Voice       voice;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    auto voiceFrames = new plaits::Voice::Frame[size];
    voice.Render(patch, modulations, voiceFrames, size);
    for(size_t i = 0; i < size; i++)
    {
        // convert short out to float
        OUT_L[i] = static_cast<float>(voiceFrames[i].out) / 32767.0f;
        OUT_R[i] = static_cast<float>(voiceFrames[i].aux) / 32767.0f;
    }
    delete[] voiceFrames;
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);
    while(1) {}
}
