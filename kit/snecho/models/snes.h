#pragma once

namespace SNES
{
struct Config
{
    float echoLength;
    float echoFeedback;
    float filter;
    float wetDry;
};

struct Modulations
{
    float echoLength;
    float echoFeedback;
    float filter;
    float wetDry;
};

class Model
{
  public:
    Model();
    void Process(float  inputLeft,
                 float  inputRight,
                 float& outputLeft,
                 float& outputRight);

    Config      cfg;
    Modulations mod;

  private:
};
} // namespace SNES