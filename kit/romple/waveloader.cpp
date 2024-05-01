#include "waveloader.h"

#include "fatfs.h"
#include "daisy_core.h"
#include "dev/sdram.h"
#include <cstddef>
#include <cstdint>

// SDRAM memory allocated using a 'bump allocator' scheme.
// the best way to use WaveLoader is by loading as much as possible up front, and using Reset() when swapping between instruments.
static constexpr size_t      sdram_bytes = 60 /*mb*/ * 1024 * 1024;
static uint8_t DSY_SDRAM_BSS g_sdram[sdram_bytes];
static size_t                g_nextAllocation = 0;

namespace daisy
{

WaveLoader::Result WaveLoader::LoadSync(const char *filename, WaveData &out)
{
    static FIL               fp_;
    static WAV_FormatTypeDef header_;
    static int32_t           workspace[kWorkspaceSize];

    if(f_open(&fp_, filename, FA_READ | FA_OPEN_EXISTING) == FR_OK)
    {
        // First Grab the Wave header info
        unsigned int br;
        f_read(&fp_, &header_, sizeof(header_), &br);
        out.data      = reinterpret_cast<float *>(g_sdram + g_nextAllocation);
        size_t length = 0;
        size_t maxLength = (sdram_bytes - g_nextAllocation) / sizeof(float);
        do
        {
            f_read(&fp_, workspace, kWorkspaceSize * sizeof(workspace[0]), &br);
            // Fill mem
            switch(header_.BitPerSample)
            {
                case 16:
                {
                    int16_t *wsp;
                    wsp = (int16_t *)workspace;
                    for(size_t i = 0; i < kWorkspaceSize * 2; i++)
                    {
                        out.data[length] = s162f(wsp[i]);
                        length++;
                    }
                }
                break;
                case 32:
                {
                    float *wsp;
                    wsp = (float *)workspace;
                    for(size_t i = 0; i < kWorkspaceSize; i++)
                    {
                        out.data[length] = wsp[i];
                        length++;
                    }
                }
                break;
                default: break;
            }
        } while(!f_eof(&fp_) || length <= maxLength - 1);
        out.length = length;
        g_nextAllocation += length * sizeof(float);
        f_close(&fp_);
    }
    else
    {
        return Result::ERR_FILE_READ;
    }
    return Result::OK;
}

void WaveLoader::Reset()
{
    g_nextAllocation = 0;
}

} // namespace daisy