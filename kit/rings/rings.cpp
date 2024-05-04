#include "daisy_patch_sm.h"
#include "dsp/part.h"
#include "dsp/strummer.h"
#include "dsp/string_synth_part.h"
#include "cv_scaler.h"

using namespace daisy;
using namespace patch_sm;

DaisyPatchSM hw;

void ProcessControls(torus::Patch* patch, torus::PerformanceState* state)
{
    //// control settings
    //cv_scaler.channel_map_[0] = controlListValueOne.GetIndex();
    //cv_scaler.channel_map_[1] = controlListValueTwo.GetIndex();
    //cv_scaler.channel_map_[2] = controlListValueThree.GetIndex();
    //cv_scaler.channel_map_[3] = controlListValueFour.GetIndex();

    ////polyphony setting
    //int poly = polyListValue.GetIndex();
    //if(old_poly != poly)
    //{
    //    part.set_polyphony(0x01 << poly);
    //    string_synth.set_polyphony(0x01 << poly);
    //}
    //old_poly = poly;

    ////model settings
    //part.set_model((torus::ResonatorModel)modelListValue.GetIndex());
    //string_synth.set_fx((torus::FxType)eggListValue.GetIndex());

    //// normalization settings
    //state->internal_note    = !noteIn;
    //state->internal_exciter = !exciterIn;
    //state->internal_strum   = !strumIn;

    ////strum
    state->strum = hw.gate_in_1.Trig();
}


uint16_t reverb_buffer[32768];

torus::CvScaler        cv_scaler;
torus::Part            part;
torus::StringSynthPart string_synth;
torus::Strummer        strummer;
bool                   easterEggOn = false;

float input[torus::kMaxBlockSize];
float output[torus::kMaxBlockSize];
float aux[torus::kMaxBlockSize];

const float kNoiseGateThreshold = 0.00003f;
float       in_level            = 0.0f;


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    const int LEFT  = 0;
    const int RIGHT = 1;
    hw.ProcessAllControls();

    torus::PerformanceState performance_state;
    torus::Patch            patch;

    ProcessControls(&patch, &performance_state);
    cv_scaler.Read(&patch, &performance_state);

    if(easterEggOn)
    {
        for(size_t i = 0; i < size; ++i)
        {
            input[i] = in[LEFT][i];
        }
        strummer.Process(NULL, size, &performance_state);
        string_synth.Process(
            performance_state, patch, input, output, aux, size);
    }
    else
    {
        // Apply noise gate.
        for(size_t i = 0; i < size; i++)
        {
            float in_sample = in[LEFT][i];
            float error, gain;
            error = in_sample * in_sample - in_level;
            in_level += error * (error > 0.0f ? 0.1f : 0.0001f);
            gain     = in_level <= kNoiseGateThreshold
                           ? (1.0f / kNoiseGateThreshold) * in_level
                           : 1.0f;
            input[i] = gain * in_sample;
        }

        strummer.Process(input, size, &performance_state);
        part.Process(performance_state, patch, input, output, aux, size);
    }

    for(size_t i = 0; i < size; i++)
    {
        out[LEFT][i]  = output[i];
        out[RIGHT][i] = aux[i];
    }
}

int main(void)
{
    hw.Init();
    float samplerate = hw.AudioSampleRate();
    float blocksize  = hw.AudioBlockSize();

    torus::InitResources();
    strummer.Init(0.01f, samplerate / blocksize);
    part.Init(reverb_buffer);
    string_synth.Init(reverb_buffer);
    cv_scaler.Init();

    hw.StartAudio(AudioCallback);

    while(1) {}
}
