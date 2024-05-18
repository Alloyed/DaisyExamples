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
        if(true)
        {
            callback(inputLeft, inputRight, mLastOutputLeft, mLastOutputRight);
        }
        outputLeft  = mLastOutputLeft;
        outputRight = mLastOutputRight;
    }

  private:
    float mLastOutputLeft;
    float mLastOutputRight;
};