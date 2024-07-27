#include "daisy_patch_sm.h"

#include "dsp/dsp.h"
#include "dsp/voice.h"

using namespace daisy;
using namespace patch_sm;

DaisyPatchSM  hw;
daisy::Switch button;
daisy::Switch toggle;

const size_t cBlockSize = 4;

// patch is used for hardware inputs; knobs/buttons and so
plaits::Patch patch;

// modulations is used for CV inputs.
plaits::Modulations modulations;

plaits::Voice        voice;
plaits::Voice::Frame voiceFrames[cBlockSize];

class Interface
{
  public:
    void UpdatePatch()
    {
        if(button.RisingEdge())
        {
            // next engine
            patch.engine = (patch.engine + 1) % plaits::kMaxEngines;
        }

        patch.note      = hw.controls[CV_1].Value();
        patch.harmonics = hw.controls[CV_2].Value();
        patch.timbre    = hw.controls[CV_3].Value();
        patch.morph     = hw.controls[CV_4].Value();

        // TODO: assign to adcs?
        patch.frequency_modulation_amount = 1.0f;
        patch.timbre_modulation_amount    = 1.0f;
        patch.morph_modulation_amount     = 1.0f;

        // I think these have a toggle mode associated
        patch.decay      = 1.0f;
        patch.lpg_colour = 1.0f;
    }
    void UpdateModulations()
    {
        CheckJackUsed();
        modulations.engine    = 0.0f;
        modulations.note      = hw.controls[CV_5].Value();
        modulations.frequency = 0.0f;
        modulations.harmonics = hw.controls[CV_6].Value();
        modulations.timbre    = hw.controls[CV_7].Value();
        modulations.morph     = hw.controls[CV_8].Value();
        modulations.trigger   = hw.gate_in_1.Trig();
        modulations.level     = 1.0f;
    }
    void CheckJackUsed()
    {
        // TODO: normal probe testing. Per MI implementation,
        // we'll need to take a pin and normal it to each jack,
        // and send out a random signal on it.
        // if the input looks like it's the random signal,
        // then mark the jack unplugged and ignore it.
        modulations.frequency_patched = false;
        modulations.timbre_patched    = true;
        modulations.morph_patched     = true;
        modulations.level_patched     = false;
    }
};
Interface interface;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    button.Debounce();
    toggle.Debounce();
    interface.UpdatePatch();
    interface.UpdateModulations();

    voice.Render(patch, modulations, voiceFrames, size);
    for(size_t i = 0; i < size; i++)
    {
        // convert short out to float
        OUT_L[i] = static_cast<float>(voiceFrames[i].out) / 32767.0f;
        OUT_R[i] = static_cast<float>(voiceFrames[i].aux) / 32767.0f;
    }
    hw.WriteCvOut(2, 2.5 * patch.morph);
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(16); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    button.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
    toggle.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());
    hw.StartAudio(AudioCallback);
    while(1) {}
}
