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
        // No anti-aliasing applied, just run the callback at a different rate
        mSampleCounter += 1.0f;
        while(mSampleCounter > mPeriod)
        {
            callback(inputLeft, inputRight, mLastOutputLeft, mLastOutputRight);
            mSampleCounter -= mPeriod;
        }
        outputLeft  = mLastOutputLeft;
        outputRight = mLastOutputRight;
    }

  private:
    float mLastOutputLeft;
    float mLastOutputRight;
    float mSampleCounter;
    float mPeriod;
};