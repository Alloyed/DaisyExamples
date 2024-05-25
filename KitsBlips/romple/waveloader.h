#pragma once
#include "util/wav_format.h"
namespace daisy
{

struct WaveData
{
    size_t length;
    float *data;
};

/** Loads a bank of wavetables into memory. 
 ** Pointers to the start of each waveform will be provided, 
 ** but the user can do whatever they want with the data once
 ** it's imported. 
 **
 ** A internal 4kB workspace is used for reading from the file, and conveting to the correct memory location. 
 **/
class WaveLoader
{
  public:
    enum class Result
    {
        OK,
        ERR_TABLE_INFO_OVERFLOW,
        ERR_FILE_READ,
        ERR_GENERIC,
    };
    WaveLoader() {}
    ~WaveLoader() {}

    /** Opens and loads the file 
     ** The data will be converted from its original type to float
     ** And the wavheader data will be stored internally to the class, 
     ** but will not be stored in the user-provided buffer.
     **
     ** Currently only 16-bit and 32-bit data is supported.
     ** The importer also assumes data is mono so stereo data will be loaded as-is 
     ** (i.e. interleaved)
     ** */
    Result LoadSync(const char *filename, WaveData &outWaveData);
    /**
     * Releases all sample memory to be re-used. any existing references are invalid after this call.
     */
    void Reset();

  private:
    static constexpr int kWorkspaceSize = 1024;
};

} // namespace daisy