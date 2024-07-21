#include "daisy_patch_sm.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/**
 * chorvs is a stereo chorus effect
 * Inputs:
 * - OUT_L: Input
 * - OUT_R: secondary input (summed to mono)
 * - CV_1: delay
 * - CV_2: LFO frequency
 * - CV_3: LFO depth
 * - CV_4: feedback
 * Outputs:
 * - OUT_L: LFO 1
 * - OUT_R: LFO 2
 */

DaisyPatchSM hw;
Switch       button, toggle;
Chorus       chorus;

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

    float delay = lerpf(
        0.0f, 1.0f, clampf(knobValue(CV_1) + jackValue(CV_5), 0.0f, 1.0f));
    float lfoFreq = lerpf(
        0.01f, 20.0f, clampf(knobValue(CV_2) + jackValue(CV_6), 0.0f, 1.0f));
    float lfoDepth = lerpf(
        0.0f, 1.0f, clampf(knobValue(CV_3) + jackValue(CV_7), 0.0f, 1.0f));
    float feedback = lerpf(
        0.0f, 1.0f, clampf(knobValue(CV_4) + jackValue(CV_8), 0.0f, 1.0f));
    // unbound: panning, per-channel controls
    chorus.SetDelay(delay);
    chorus.SetLfoFreq(lfoFreq);
    chorus.SetLfoDepth(lfoDepth);
    chorus.SetFeedback(feedback);

    for(size_t i = 0; i < size; i++)
    {
        // summing mixdown. if right is normalled to left, acts as a mono signal.
        float inputFloat = (IN_L[i] + IN_R[i]) * 0.5f;
        chorus.Process(inputFloat);
        OUT_L[i] = chorus.GetLeft();
        OUT_R[i] = chorus.GetRight();
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    chorus.Init(hw.AudioCallbackRate());
    button.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
    toggle.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());

    hw.StartAudio(AudioCallback);
    while(1) {}
}
