#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;


// 3-band EQ adapted from
// https://www.musicdsp.org/en/latest/Filters/236-3-band-equaliser.html

class Equalizer
{
    static constexpr float vsa
        = (1.0 / 4294967295.0); // Very small amount (Denormal Fix)
  public:
    // ---------------
    //| Initialise EQ |
    // ---------------

    // Recommended frequencies are ...
    //
    //  lowfreq  = 880  Hz
    //  highfreq = 5000 Hz
    //
    // Set mixfreq to whatever rate your system is using (eg 48Khz)

    Equalizer()
    {
        // Set Low/Mid/High gains to unity
        lg = 1.0;
        mg = 1.0;
        hg = 1.0;
    }

    void SetFrequencies(float lowfreq, float highfreq, float sampleRate)
    {
        // Calculate filter cutoff frequencies
        lf = 2 * sin(M_PI * ((float)lowfreq / (float)sampleRate));
        hf = 2 * sin(M_PI * ((float)highfreq / (float)sampleRate));
    }

    void SetGain(float lowGain, float highGain)
    {
        lg = lowGain;
        hg = highGain;
    }

    // ---------------
    //| EQ one sample |
    // ---------------

    // - sample can be any range you like :)
    //
    // Note that the output will depend on the gain settings for each band
    // (especially the bass) so may require clipping before output, but you
    // knew that anyway :)

    float Process(float sample)
    {
        // Locals

        float l, m, h; // Low / Mid / High - Sample Values

        // Filter #1 (lowpass)

        f1p0 += (lf * (sample - f1p0)) + vsa;
        f1p1 += (lf * (f1p0 - f1p1));
        f1p2 += (lf * (f1p1 - f1p2));
        f1p3 += (lf * (f1p2 - f1p3));

        l = f1p3;

        // Filter #2 (highpass)

        f2p0 += (hf * (sample - f2p0)) + vsa;
        f2p1 += (hf * (f2p0 - f2p1));
        f2p2 += (hf * (f2p1 - f2p2));
        f2p3 += (hf * (f2p2 - f2p3));

        h = sdm3 - f2p3;

        // Calculate midrange (signal - (low + high))

        m = sdm3 - (h + l);

        // Scale, Combine and store

        l *= lg;
        m *= mg;
        h *= hg;

        // Shuffle history buffer

        sdm3 = sdm2;
        sdm2 = sdm1;
        sdm1 = sample;

        // Return result

        return (l + m + h);
    }

  private:
    // Filter #1 (Low band)
    float lf;   // Frequency
    float f1p0; // Poles ...
    float f1p1;
    float f1p2;
    float f1p3;

    // Filter #2 (High band)

    float hf;   // Frequency
    float f2p0; // Poles ...
    float f2p1;
    float f2p2;
    float f2p3;

    // Sample history buffer

    float sdm1; // Sample data minus 1
    float sdm2; //                   2
    float sdm3; //                   3

    // Gain Controls

    float lg; // low  gain
    float mg; // mid  gain
    float hg; // high gain
};

DaisyPatchSM hw;
Equalizer    eqLeft;
Equalizer    eqRight;


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

    float lowFreq = lerpf(
        0.0f, 880.0f, clampf(knobValue(CV_1) + jackValue(CV_5), 0.0f, 1.0f));
    float highFreq
        = lerpf(1000.0f,
                10000.0f,
                clampf(knobValue(CV_2) + jackValue(CV_6), 0.0f, 1.0f));
    float lowGain = lerpf(
        0.0f, 1.5f, clampf(knobValue(CV_3) + jackValue(CV_7), 0.0f, 1.0f));
    float highGain = lerpf(
        0.0f, 1.5f, clampf(knobValue(CV_4) + jackValue(CV_8), 0.0f, 1.0f));

    eqLeft.SetFrequencies(lowFreq, highFreq, hw.AudioSampleRate());
    eqLeft.SetGain(lowGain, highGain);
    eqRight.SetFrequencies(lowFreq, highFreq, hw.AudioSampleRate());
    eqRight.SetGain(lowGain, highGain);
    for(size_t i = 0; i < size; i++)
    {
        OUT_L[i] = eqLeft.Process(IN_L[i]);
        OUT_R[i] = eqRight.Process(IN_R[i]);
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
