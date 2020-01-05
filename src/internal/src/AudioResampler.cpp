// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/AudioResampler.h"
#include "internal/Assertions.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/Macros.h"

#include <algorithm>

using namespace std;

namespace lab
{

const double AudioResampler::MaxRate = 8.0;

void AudioResampler::process(ContextRenderLock & r, AudioSourceProvider * provider, AudioBus * destinationBus)
{
    ASSERT(provider);
    if (!provider)
        return;

    int source_channels = provider->numberOfChannels();
    int dest_channels = destinationBus->numberOfChannels();
    if (!source_channels || !dest_channels)
        return;

    // ensure there are at least enough kernels to deal with the resampling required
    int process_count = source_channels < dest_channels? source_channels : dest_channels;
    for (int i = m_kernels.size(); i < process_count; ++i)
        m_kernels.push_back(std::unique_ptr<AudioResamplerKernel>(new AudioResamplerKernel(this)));

    // Setup the source bus.
    int channel;
    uint32_t framesToProcess = r.context()->currentFrames();
    for (channel = 0; channel < process_count; ++channel)
    {
        // Calculate a pointer to a buffer, and the number of bytes the kernel needs fetched
        size_t framesNeeded;
        float * fillPointer = m_kernels[channel]->getSourcePointer(framesToProcess, &framesNeeded);
        if (!fillPointer)
            break;

        provider->provideInput(channel, fillPointer, framesNeeded);
    }

    // Resample each channel to the destination. The loop is bounded by the number of channels copied
    // in the set up stage.
    for (int i = 0; i < channel; ++i)
    {
        float * destination = destinationBus->channel(i)->mutableData();
        m_kernels[i]->process(r, destination);
    }
}

void AudioResampler::setRate(double rate)
{
    if (std::isnan(rate) || std::isinf(rate) || rate <= 0.0)
        return;

    m_rate = min(AudioResampler::MaxRate, rate);
}

void AudioResampler::reset()
{
    size_t numberOfChannels = m_kernels.size();
    for (size_t i = 0; i < numberOfChannels; ++i)
        m_kernels[i]->reset();
}

}  // namespace lab
