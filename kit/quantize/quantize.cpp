#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "ScaleQuantizer.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/**
 * Quantizer module:
 * Inputs:
 * - C5  CV_1: v/oct input (-5, 5)
 * - C4  CV_2: v/oct transpose (-5, 5) (centered to c4 by default)
 * - C3  CV_3: scale
 * - B7  BUTTON : select transpose root
 * Outputs:
 * - B2  OUT_L: v/oct output
 * - B5  GATE_OUT_1: 10ms trigger on note change
 * Improvements:
 * - make v/oct inputs 0-10v instead (passive mixer with -5v ref in?)
 * - use encoder+led array for scale selection?
 * - built in sample+hold? (ala disting)
 * - midi in for custom scales?
 */

DaisyPatchSM   hw;
Switch         updateTransposeRoot;
ScaleQuantizer quantizer(kChromaticScale, DSY_COUNTOF(kChromaticScale));

struct State
{
    // in
    float input;
    float transpose;
    float scale;

    // out
    int   hasNoteChanged;
    float lastNote;

    // state
    float transposeRoot;
};
State state;

void Controls();
void CvCallback(uint16_t **output, size_t size)
{
    hw.ProcessAllControls();
    Controls();

    float inputNote = fmap(state.input, 0, 60);
    float transposeNote
        = fmap(state.transpose, 0, 60) - fmap(state.transposeRoot, 0, 60);

    float quantizedNote = quantizer.Process(inputNote);
    if(state.lastNote != quantizedNote)
    {
        state.hasNoteChanged++;
        state.lastNote = quantizedNote;
    }
    float noteOut = fclamp(quantizedNote + transposeNote, 0.f, 127.f) / 127.0f;
    for(size_t i = 0; i < size; i++)
    {
        output[0][i] = noteOut;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.StartDac(CvCallback);
    while(1)
    {
        if(state.hasNoteChanged > 0)
        {
            // trigger out
            // note changes are queued in case they happen within the 10ms window
            state.hasNoteChanged--;
            dsy_gpio_write(&hw.gate_out_1, true);
            hw.Delay(9);
            dsy_gpio_write(&hw.gate_out_1, false);
            hw.Delay(1);
        }
    }
}

void Controls()
{
    updateTransposeRoot.Debounce();

    // convert to 0-1
    state.input     = (hw.controls[CV_1].Value() + 1.0f) * 0.5f;
    state.transpose = (hw.controls[CV_2].Value() + 1.0f) * 0.5f;
    if(updateTransposeRoot.Pressed())
    {
        state.transposeRoot = state.transpose;
    }
    state.scale = hw.controls[CV_3].Value();
}
