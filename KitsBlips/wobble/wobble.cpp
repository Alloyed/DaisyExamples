#include "daisy_patch_sm.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/**
 * wobble is a set of 2 lfos, optionally clock-synced:
 * Inputs:
 * - GATE_IN_1: LFO 1 clock
 * - GATE_IN_2: LFO 2 clock
 * - CV_1: Clock 1 divide rate
 * - CV_2: Clock 2 divide rate
 * - CV_3: Clock 1 waveform
 * - CV_4: Clock 2 waveform
 * - B8: Toggle LED
 * Outputs:
 * - OUT_L: LFO 1
 * - OUT_R: LFO 2
 * - GATE_OUT_1: LFO 1 reset
 * - GATE_OUT_2: LFO 2 reset
 * - CV_OUT_2: LED out
 */

/**
 * TODO:
 * - clock sync
 * - inline LFO implementation for better phase control
 */

DaisyPatchSM hw;
Switch       button, toggle;
int32_t      lastClock1 = 0;
int32_t      lastClock2 = 0;
float        phase1     = 0.0f;
float        phase2     = 0.0f;
float        add1       = 0.0001f;
float        add2       = 0.0001f;

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

float shapeLfo(float phase, int8_t shape)
{
    // ordered by personal preference
    switch(shape)
    {
        case 0:
        {
            // triangle
            return phase < 0.5 ? phase * 2.0f : 1 - ((phase - 0.5f) * 2.0f);
        }
        case 1:
        {
            // reverse ramp
            return 1 - phase;
        }
        case 2:
        {
            // square
            return phase < 0.5 ? 1.0f : 0.0f;
        }
        case 3:
        {
            // sin
            return (cosf(phase * PI_F * 2.0f) + 1.0f) * 0.5f;
        }
        case 4:
        {
            // ramp
            return phase;
        }
    }
    return phase;
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    button.Debounce();
    toggle.Debounce();

    float div1 = lerpf(
        1.0f, 16.0f, clampf(knobValue(CV_1) + jackValue(CV_5), 0.0f, 1.0f));
    float div2 = lerpf(
        1.0f, 16.0f, clampf(knobValue(CV_2) + jackValue(CV_6), 0.0f, 1.0f));
    float shape1 = lerpf(
        0.0f, 4.9f, clampf(knobValue(CV_3) + jackValue(CV_7), 0.0f, 1.0f));
    float shape2 = lerpf(
        0.0f, 4.9f, clampf(knobValue(CV_4) + jackValue(CV_8), 0.0f, 1.0f));


    if(button.RisingEdge())
    {
        // reset phase
        phase1 = 0.0f;
        phase2 = 0.0f;
    }
    if(hw.gate_in_1.Trig())
    {
        add1       = floorf(div1) / lastClock1;
        lastClock1 = 0;
        phase1     = 0.0f;
    }
    if(hw.gate_in_2.Trig())
    {
        add2       = floorf(div2) / lastClock2;
        lastClock2 = 0;
        phase2     = 0.0f;
    }

    for(size_t i = 0; i < size; i++)
    {
        lastClock1++;
        lastClock2++;
        phase1   = fmodf(phase1 + add1, 1.0f);
        phase2   = fmodf(phase2 + add2, 1.0f);
        OUT_L[i] = -shapeLfo(phase1, static_cast<int8_t>(shape1));
        OUT_R[i] = -shapeLfo(phase2, static_cast<int8_t>(shape2));
        if(i == size - 1)
        {
            size_t idx = toggle.Pressed() ? 1 : 0;
            hw.WriteCvOut(CV_OUT_2, (out[idx][i]) * 2.5f);
        }
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(1); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);

    button.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
    toggle.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());

    hw.StartAudio(AudioCallback);
    while(1) {}
}
