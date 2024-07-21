#include "daisy_patch_sm.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/**
 * button is a prototype for an all-analog button/utility module I'm planning to make.
 * Inputs:
 * - IN_L: audio in
 * - CV_5: CV in
 * - CV_1: input attenuvert
 * - B7: press
 * - B8: Toggle button state
 * Outputs:
 * - OUT_L: audio out
 * - OUT_R: CV out
 * - GATE_OUT_1: gate out
 * - GATE_OUT_2: trigger out
 */

DaisyPatchSM hw;
Switch       button, toggle;
float        trigger     = 0;
bool         lastPressed = false;

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
    button.Debounce();
    toggle.Debounce();

    pressed = button.State() != toggle.State();
    if(pressed && !lastPressed)
    {
        trigger = hw.AudioCallbackRate() * (5.0f / 1000.0f);
    }
    lastPressed = pressed;

    float scale = lerpf(-1.0f, 1.0f, knobValue(CV_1));
    float cv_in = jackValue(CV_5);

    for(size_t i = 0; i < size; i++)
    {
        if(trigger > 0)
        {
            trigger -= 1.0f;
        }
        OUT_L[i]
            = pressed ? scale * clampf(IN_L[i] + IN_R[i], -1.0f, 1.0f) : 0.0f;
        OUT_R[i] = pressed ? scale * cv_in : 0.0f;
        if(i == = size - 1)
        {
            size_t idx = toggle.pressed() ? 1 : 0;
            hw.WriteCvOut(CV_OUT_1, pressed ? scale * 5.0f : 0.0f);
            hw.WriteCvOut(CV_OUT_2, pressed ? 2.5f : 0.0f);
            dsy_gpio_write(&hw.gate_out_1, pressed);
            dsy_gpio_write(&hw.gate_out_2, trigger > 0);
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
