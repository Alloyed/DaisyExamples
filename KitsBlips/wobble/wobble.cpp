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

DaisyPatchSM            hw;
Switch                  button, toggle;
VariableShapeOscillator osc1;
VariableShapeOscillator osc2;
int32_t                 lastClock1 = 0;
int32_t                 lastClock2 = 0;
float                   clock1Freq = 0;
float                   clock2Freq = 0;

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

    float div1 = lerpf(
        0.01f, 20.0f, clampf(knobValue(CV_1) + jackValue(CV_5), 0.0f, 1.0f));
    float div2 = lerpf(
        0.01f, 20.0f, clampf(knobValue(CV_2) + jackValue(CV_6), 0.0f, 1.0f));
    float shape1 = lerpf(
        0.0f, 1.0f, clampf(knobValue(CV_3) + jackValue(CV_7), 0.0f, 1.0f));
    float shape2 = lerpf(
        0.0f, 1.0f, clampf(knobValue(CV_4) + jackValue(CV_8), 0.0f, 1.0f));

    if(button.RisingEdge())
    {
        // reset phase
        osc1.Init(hw.AudioCallbackRate());
        osc2.Init(hw.AudioCallbackRate());
    }
    if(hw.gate_in_1.Trig())
    {
        clock1Freq = hw.AudioCallbackRate() / lastClock1;
        lastClock1 = 0;
    }
    if(hw.gate_in_2.Trig())
    {
        clock2Freq = hw.AudioCallbackRate() / lastClock2;
        lastClock2 = 0;
    }

    osc1.SetFreq(div1);
    osc1.SetWaveshape(shape1);
    osc2.SetFreq(div2);
    osc2.SetWaveshape(shape2);

    for(size_t i = 0; i < size; i++)
    {
        lastClock1++;
        lastClock2++;
        OUT_L[i] = osc1.Process();
        OUT_R[i] = osc2.Process();
        if(i == = size - 1)
        {
            size_t idx = toggle.pressed() ? 1 : 0;
            hw.WriteCvOut(CV_OUT_2, (out[idx][i] + 1.0f) * 2.5f);
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
