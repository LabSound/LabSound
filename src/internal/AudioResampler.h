// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioResampler_h
#define AudioResampler_h

#include "LabSound/core/AudioSourceProvider.h"

#include "internal/AudioBus.h"
#include "internal/AudioResamplerKernel.h"

#include <vector>

namespace lab {

// AudioResampler resamples the audio stream from an AudioSourceProvider.
// The audio stream may be single or multi-channel.
// The default constructor defaults to single-channel (mono).

class AudioResampler {
public:
    AudioResampler();
    AudioResampler(unsigned numberOfChannels);
    ~AudioResampler() { }
    
    // Given an AudioSourceProvider, process() resamples the source stream into destinationBus.
    void process(ContextRenderLock&, AudioSourceProvider*, AudioBus* destinationBus, size_t framesToProcess);

    // Resets the processing state.
    void reset();

    void configureChannels(unsigned numberOfChannels);

    // 0 < rate <= MaxRate
    void setRate(double rate);
    double rate() const { return m_rate; }

    static const double MaxRate;

private:
    double m_rate;
    std::vector<std::unique_ptr<AudioResamplerKernel> > m_kernels;
    std::unique_ptr<AudioBus> m_sourceBus;
};

} // namespace lab

#endif // AudioResampler_h
