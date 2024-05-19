#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "waveloader.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM    hw;
WaveLoader      loader;
SdmmcHandler    sdcard;
FatFSInterface  fsInterface;
VoctCalibration calibration1;

/* loads each sample as a one-shot, triggered by gate inputs.*/
class TriggerSampler
{
  public:
    TriggerSampler() {}
    ~TriggerSampler() {}

    void Load(const char* filename, GateIn gateIn)
    {
        loader.LoadSync(filename, m_sample);
        m_gate = gateIn;
    }

    float Process()
    {
        if(m_gate.Trig())
        {
            m_playing         = true;
            m_nextSampleIndex = 0;
        }

        if(!m_playing)
        {
            return 0.0f;
        }
        float sample = m_sample.data[m_nextSampleIndex];
        m_nextSampleIndex++;
        if(m_nextSampleIndex >= m_sample.length)
        {
            m_playing = false;
        }
        return sample;
    }

  private:
    WaveData m_sample;
    GateIn   m_gate;
    bool     m_playing;
    size_t   m_nextSampleIndex;
};

enum class InterpolationMode
{
    Floor,  // always pick the last sample
    Linear, // assume a line in between the last sample and the next
};

/* loads a single sample and pitches it up and down to match 1v/oct.
 * a slight volume envelope is applied: lowering the gate will stop the sample.
 */
template <InterpolationMode mode>
class PitchSampler
{
  public:
    PitchSampler() {}
    ~PitchSampler() {}

    void Load(const char* filename, GateIn gateInput, AnalogControl cvInput)
    {
        loader.LoadSync(filename, m_sample);
        // TODO: pull m_baseFreq off of sample name
        m_baseFreq = mtof(60); // C4
        // TODO: pull loop points off of wav file, if available
        m_shouldLoop = false;

        m_gate         = gateInput;
        m_pitchControl = cvInput;

        m_envelope.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
        m_envelope.SetAttackTime(.01f, 0.0f);
        m_envelope.SetSustainLevel(1.0f);
        m_envelope.SetDecayTime(.01f);
        m_envelope.SetReleaseTime(.01f);
    }

    float Process()
    {
        // input
        if(m_gate.Trig())
        {
            m_playing                = true;
            m_playHead               = 0.0f; // play from start
            const bool resetEnvelope = true;
            m_envelope.Retrigger(resetEnvelope);
        }
        float inNote = calibration1.ProcessInput(m_pitchControl.Process());

        // processing
        if(!m_playing)
        {
            return 0.0f;
        }

        float targetFreq = mtof(inNote);
        float playSpeed  = targetFreq / m_baseFreq;

        float sample;
        switch(mode)
        {
            case InterpolationMode::Floor:
            {
                sample = m_sample.data[static_cast<size_t>(m_playHead)];
            }
            break;
            case InterpolationMode::Linear:
            {
                size_t flooredPlayHead = static_cast<size_t>(m_playHead);
                float  difference      = m_playHead - flooredPlayHead;
                float  lastSample      = m_sample.data[flooredPlayHead];
                float  nextSample
                    = m_sample
                          .data[std::min(m_sample.length, flooredPlayHead + 1)];
                sample = lastSample + (nextSample - lastSample) * difference;
            }
            break;
        }
        sample *= m_envelope.Process(m_gate.State());
        m_playHead += playSpeed;
        if(m_playHead >= m_sample.length)
        {
            if(m_shouldLoop)
            {
                m_playHead -= m_sample.length;
            }
            else
            {
                m_playing = false;
            }
        }

        return sample;
    }
    daisysp::Adsr m_envelope;
    WaveData      m_sample;
    AnalogControl m_pitchControl;
    GateIn        m_gate;
    bool          m_playing;
    bool          m_shouldLoop; // TODO loop points
    float         m_playHead;
    float         m_baseFreq;
};

/*
 * loads a single sample and pitches it up and down to match 1v/oct.
 * always loops, phase is never reset.
 */
class OscSampler
{
  public:
    OscSampler() {}
    ~OscSampler() {}

    void Load(const char* folder) {}

    float    Process() { return 0.0f; }
    WaveData m_wavetable;
};

enum ADCChannels
{
    PitchCv,
    Size,
};

enum MountMode
{
    Delay     = 0,
    Immediate = 1,
};

// Sampler configuration
// TODO: runtime config?
PitchSampler<InterpolationMode::Floor> sampler1;
PitchSampler<InterpolationMode::Floor> sampler2;

int main(void)
{
    hw.Init();

    // mount sdcard to "/"
    SdmmcHandler::Config sdcardConfig;
    sdcardConfig.Defaults();
    sdcard.Init(sdcardConfig);
    fsInterface.Init(FatFSInterface::Config::MEDIA_SD);
    f_mount(&fsInterface.GetSDFileSystem(), "/", MountMode::Immediate);

    // out
    hw.SetAudioBlockSize(4); // number of samples handled per callback
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);


    // configure channels
    sampler1.Load("/sample1.wav", hw.gate_in_1, hw.controls[CV_1]);
    sampler2.Load("/sample1.wav", hw.gate_in_2, hw.controls[CV_2]);

    // main loop
    hw.StartAudio(
        [](AudioHandle::InputBuffer  in,
           AudioHandle::OutputBuffer out,
           size_t                    size)
        {
            hw.ProcessAllControls();
            for(size_t i = 0; i < size; i++)
            {
                OUT_L[i] = sampler1.Process();
                OUT_R[i] = sampler2.Process();
            }
        });

    while(1) {}
}
