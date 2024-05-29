#pragma once

#include <cstdint>

class Resampler
{
  public:
    Resampler(int32_t sourceRate, int32_t targetRate);

    template <typename F>
    void Process(float  inputLeft,
                 float  inputRight,
                 float& outputLeft,
                 float& outputRight,
                 F&&    callback)
    {
        mSampleCounter += 1.0f;
        while(mSampleCounter > mPeriod)
        {
            // TODO: we should filter the inputs to avoid aliasing
            // https://dspguru.com/dsp/faqs/multirate/decimation/
            float decimatedInputLeft  = inputLeft;
            float decimatedInputRight = inputRight;

            mLastOutputLeft  = mNextOutputLeft;
            mLastOutputRight = mNextOutputRight;
            callback(inputLeft, inputRight, mNextOutputLeft, mNextOutputRight);
            mSampleCounter -= mPeriod;
        }
        float t = mSampleCounter / mPeriod;

        // Linear interpolation
        outputLeft  = lerpf(mLastOutputLeft, mNextOutputLeft, t);
        outputRight = lerpf(mLastOutputRight, mNextOutputRight, t);
    }

  private:
    float mLastOutputLeft;
    float mLastOutputRight;
    float mNextOutputLeft;
    float mNextOutputRight;
    float mSampleCounter;
    float mPeriod;
};