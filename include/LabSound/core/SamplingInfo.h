
#ifndef lab_sampling_info_h
#define lab_sampling_info_h

#include <chrono>
#include <stdint.h>

// low bit of current_sample_frame indexes time point 0 or 1
// (so that time and epoch are written atomically, after the alternative epoch has been filled in)
struct SamplingInfo
{
    uint64_t current_sample_frame{0};
    double current_time{0.0};
    float sampling_rate{0.f};
    std::chrono::high_resolution_clock::time_point epoch[2];
};

#endif
