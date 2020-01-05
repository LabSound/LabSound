// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioResampler_h
#define AudioResampler_h

#include "LabSound/core/AudioSourceProvider.h"
#include "LabSound/core/AudioBus.h"

#include "internal/AudioResamplerKernel.h"

#include <vector>

namespace lab {

// AudioResampler resamples the audio stream from an AudioSourceProvider.

class AudioResampler {
public:
    AudioResampler() = default;
    ~AudioResampler() = default;

    // Given an AudioSourceProvider, process() resamples the source stream into destinationBus.
    void process(ContextRenderLock&, AudioSourceProvider*, AudioBus* destinationBus);

    // Resets the processing state.
    void reset();

    // 0 < rate <= MaxRate
    void setRate(double rate);
    double rate() const { return m_rate; }

    static const double MaxRate;

private:
    double m_rate = 1.f;
    std::vector<std::unique_ptr<AudioResamplerKernel> > m_kernels;
};

} // namespace lab

#endif // AudioResampler_h
