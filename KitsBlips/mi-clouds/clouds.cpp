#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "granular_processor.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM            hw;
GranularProcessorClouds processor;
Parameters*             parameters;

// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128];

void Controls();
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    Controls();

    FloatFrame input[size];
    FloatFrame output[size];

    for(size_t i = 0; i < size; i++)
    {
        input[i].l  = IN_L[i];
        input[i].r  = IN_R[i];
        output[i].l = output[i].r = 0.f;
    }

    processor.Process(input, output, size);

    for(size_t i = 0; i < size; i++)
    {
        OUT_L[i] = output[i].l;
        OUT_R[i] = output[i].r;
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(32); // clouds won't work with blocks bigger than 32
    float sample_rate = hw.AudioSampleRate();

    //init the luts
    InitResources(sample_rate);

    processor.Init(sample_rate,
                   block_mem,
                   sizeof(block_mem),
                   block_ccm,
                   sizeof(block_ccm));

    parameters = processor.mutable_parameters();

    hw.StartAudio(AudioCallback);
    while(1)
    {
        processor.Prepare();
    }
}

void Controls()
{
    hw.ProcessAllControls();

    ////process knobs
    //for(int i = 0; i < 4; i++)
    //{
    //    paramControls[i].Process();
    //}

    ////long press switch page
    //menupage += hw.encoder.TimeHeldMs() > 1000.f && !held;
    //menupage %= NUM_PAGES;
    //held = hw.encoder.TimeHeldMs() > 1000.f; //only change pages once
    //held &= hw.encoder.Pressed();            //reset on release
    //selected &= !held;                       //deselect on page change

    //selected ^= hw.encoder.RisingEdge();

    ////encoder turn
    //if(selected)
    //{
    //    if(menupage == 0)
    //    {
    //        increment += hw.encoder.Increment();
    //    }
    //    else
    //    {
    //        switch(cursorpos)
    //        {
    //            case 0: freeze_btn ^= abs(hw.encoder.Increment()); break;
    //            case 1:
    //                pbMode += hw.encoder.Increment();
    //                pbMode = mymod(pbMode, 4);
    //                processor.set_playback_mode((PlaybackMode)pbMode);
    //                break;
    //            case 2:
    //                quality += hw.encoder.Increment();
    //                quality = mymod(quality, 4);
    //                processor.set_quality(quality);
    //                break;
    //        }
    //    }
    //}
    //else
    //{
    //    if(increment != 0)
    //    {
    //        paramControls[cursorpos].incParamNum(increment);
    //        increment = 0;
    //    }
    //    cursorpos += hw.encoder.Increment();
    //    cursorpos = mymod(cursorpos, menupage ? 3 : 4);
    //}

    //// gate ins
    //parameters->freeze  = hw.gate_input[0].State() || freeze_btn;
    //parameters->trigger = hw.gate_input[1].Trig();
    //parameters->gate    = hw.gate_input[1].State();
}
