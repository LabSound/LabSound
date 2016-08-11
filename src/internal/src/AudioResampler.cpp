// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/AudioResampler.h"
#include "internal/AudioBus.h"

#include <algorithm>
#include <WTF/MathExtras.h>

using namespace std;
 
namespace lab {

const double AudioResampler::MaxRate = 8.0;

AudioResampler::AudioResampler()
    : m_rate(1.0)
{
    m_kernels.push_back(std::unique_ptr<AudioResamplerKernel>(new AudioResamplerKernel(this)));
    m_sourceBus = std::unique_ptr<AudioBus>(new AudioBus(1, 0, false));
}

AudioResampler::AudioResampler(unsigned numberOfChannels)
    : m_rate(1.0)
{
    for (unsigned i = 0; i < numberOfChannels; ++i)
        m_kernels.push_back(std::unique_ptr<AudioResamplerKernel>(new AudioResamplerKernel(this)));

    m_sourceBus = std::unique_ptr<AudioBus>(new AudioBus(numberOfChannels, 0, false));
}

void AudioResampler::configureChannels(unsigned numberOfChannels)
{
    unsigned currentSize = m_kernels.size();
    if (numberOfChannels == currentSize)
        return; // already setup

    // First deal with adding or removing kernels.
    if (numberOfChannels > currentSize) {
        for (unsigned i = currentSize; i < numberOfChannels; ++i)
            m_kernels.push_back(std::unique_ptr<AudioResamplerKernel>(new AudioResamplerKernel(this)));
    } else
        m_kernels.resize(numberOfChannels);

    // Reconfigure our source bus to the new channel size.
    m_sourceBus = std::unique_ptr<AudioBus>(new AudioBus(numberOfChannels, 0, false));
}

void AudioResampler::process(ContextRenderLock& r, AudioSourceProvider* provider, AudioBus* destinationBus, size_t framesToProcess)
{
    ASSERT(provider);
    if (!provider)
        return;
        
    unsigned numberOfChannels = m_kernels.size();

    // Make sure our configuration matches the bus we're rendering to.
    bool channelsMatch = (destinationBus && destinationBus->numberOfChannels() == numberOfChannels);
    ASSERT(channelsMatch);
    if (!channelsMatch)
        return;

    // Setup the source bus.
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        // Figure out how many frames we need to get from the provider, and a pointer to the buffer.
        size_t framesNeeded;
        float* fillPointer = m_kernels[i]->getSourcePointer(framesToProcess, &framesNeeded);
        ASSERT(fillPointer);
        if (!fillPointer)
            return;
            
        m_sourceBus->setChannelMemory(i, fillPointer, framesNeeded);
    }

    // Ask the provider to supply the desired number of source frames.
    provider->provideInput(m_sourceBus.get(), m_sourceBus->length());

    // Now that we have the source data, resample each channel into the destination bus.
    // FIXME: optimize for the common stereo case where it's faster to process both left/right channels in the same inner loop.
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        float* destination = destinationBus->channel(i)->mutableData();
        m_kernels[i]->process(r, destination, framesToProcess);
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
    unsigned numberOfChannels = m_kernels.size();
    for (unsigned i = 0; i < numberOfChannels; ++i)
        m_kernels[i]->reset();
}

} // namespace lab
