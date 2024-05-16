#include "daisy_patch_sm.h"
#include "daisysp.h"

#include "models/snes.h"
#include "models/psx.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

#define SAMPLE_RATE 32000

DaisyPatchSM     hw;
SNES::Model      snes;
constexpr size_t bufSize = PSX::Model::GetBufferDesiredSizeFloats(SAMPLE_RATE);
float            psxBuffer[bufSize];
PSX::Model       psx(SAMPLE_RATE, psxBuffer, sizeof(psxBuffer));

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    for(size_t i = 0; i < size; i++)
    {
        //psx.Process(IN_L[i], IN_R[i], OUT_L[i], OUT_R[i]);
        snes.Process(IN_L[i], IN_R[i], OUT_L[i], OUT_R[i]);
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(8); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);
    hw.StartAudio(AudioCallback);
    while(1) {}
}
