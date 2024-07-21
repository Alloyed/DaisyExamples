#include "daisy_patch_sm.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/**
 * follow is an envelope follower. pitch detection too, eventually(?)
 * Inputs:
 * - CV_1: Env Rise
 * - CV_2: Env Fall
 * - CV_3: Gain
 * - IN_L: audio in
 * Outputs:
 * - OUT_L: env out
 * - GATE_OUT_1: env to gate
 */

DaisyPatchSM hw;
Switch       button, toggle;
float        state = 0.0f;

inline constexpr float lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

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

    float rise = lerpf(
        0.3f, 0.0001f, clampf(knobValue(CV_1) + jackValue(CV_5), 0.0f, 1.0f));
    float fall = lerpf(0.3f,
                       0.0000001f,
                       clampf(knobValue(CV_2) + jackValue(CV_6), 0.0f, 1.0f));
    float gain = lerpf(
        0.8f, 1.5f, clampf(knobValue(CV_3) + jackValue(CV_7), 0.0f, 1.0f));

    if(button.RisingEdge())
    {
        // reset phase
        osc1.Init(hw.AudioCallbackRate());
        osc2.Init(hw.AudioCallbackRate());
    }

    for(size_t i = 0; i < size; i++)
    {
        float in = fabsf(IN_L[i]) * gain;
        // TODO: not sample rate independent
        state    = lerpf(state, in, in > state ? rise : fall);
        OUT_L[i] = state;
        if(i == size - 1)
        {
            hw.WriteCvOut(CV_OUT_2, OUT_L[i] * 5.0f);
            dsy_gpio_write(&hw.gate_out_1, OUT_L[i] > 0.3f);
        }
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    osc1.Init(hw.AudioCallbackRate());
    osc2.Init(hw.AudioCallbackRate());
    button.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
    toggle.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());

    hw.StartAudio(AudioCallback);
    while(1) {}
}
