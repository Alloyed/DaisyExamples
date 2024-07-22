#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;

inline constexpr float clampf(float in, float min, float max)
{
    return in > max ? max : in < min ? min : in;
}


float knobValue(int32_t cvEnum)
{
    return clampf(hw.controls[cvEnum].Value(), 0.0f, 1.0f);
}

float jackValue(int32_t cvEnum)
{
    return clampf(hw.controls[cvEnum].Value(), -1.0f, 1.0f);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();


    hw.WriteCvOut(1, 2.5 * knobValue(CV_4));
    hw.WriteCvOut(2, 2.5 * knobValue(CV_4));
    for(size_t i = 0; i < size; i++)
    {
        OUT_L[i] = knobValue(CV_4);
        OUT_R[i] = knobValue(CV_4);
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartAudio(AudioCallback);
    while(1) {}
}
