#include "dsp/resampler.h"

Resampler::Resampler(int32_t sourceRate, int32_t targetRate)
: mPeriod(static_cast<float>(sourceRate) / static_cast<float>(targetRate))
{
}