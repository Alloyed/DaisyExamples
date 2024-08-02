#include "daisy_patch_sm.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/**
 * tracker is a 1-track sequencer with a tracker-style interface.
 * Inputs:
 * - C5 CV_1: X in
 * - C4 CV_2: Y in
 * - C3 CV_3: Pattern in
 * - B10 GATE_IN_1: clock in
 * - B7: toggle play
 * - B8: column
 * - D1,D8,D9,D10: display
 * - D2,D3,D4: encoder
 * - A8,A9: MIDI in
 * Outputs:
 * - B2 OUT_L: 1v/oct out
 * - B5 GATE_OUT_1: clock out
 * - B6 GATE_OUT_2: sequence gate out
 * Unused:
 * GPIO A2 A3 D5 D6 D7
 * in_l, in_r
 * out_r
 * CV 4 5 6 7 8
 * CV_out 1 2
 * gate_in 2
 */

using Display = OledDisplay<SSD130x4WireSpi128x64Driver>;

DaisyPatchSM      hw;
Switch            playButton, columnButton;
Encoder           encoder;
MidiUartTransport midi;
Display           display;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    playButton.Debounce();
    columnButton.Debounce();

    for(size_t i = 0; i < size; i++) {}
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    playButton.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
    columnButton.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());

    /** Configure the Display */
    Display::Config displayCfg;
    displayCfg.driver_config.transport_config.pin_config.dc = DaisyPatchSM::D5;
    displayCfg.driver_config.transport_config.pin_config.reset
        = DaisyPatchSM::D6;
    /** And Initialize */
    display.Init(displayCfg);

    message_idx = 0;
    char strbuff[128];

    hw.StartAudio(AudioCallback);

    while(1)
    {
        System::Delay(500);
        switch(message_idx)
        {
            case 0: sprintf(strbuff, "Testing. . ."); break;
            case 1: sprintf(strbuff, "Daisy. . ."); break;
            case 2: sprintf(strbuff, "1. . ."); break;
            case 3: sprintf(strbuff, "2. . ."); break;
            case 4: sprintf(strbuff, "3. . ."); break;
            default: break;
        }
        message_idx = (message_idx + 1) % 5;
        display.Fill(true);
        display.SetCursor(0, 0);
        display.WriteString(strbuff, Font_6x8, false);
        display.Update();
    }
}
