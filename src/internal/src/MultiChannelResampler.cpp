// License: BSD 3 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "internal/MultiChannelResampler.h"
#include "internal/AudioBus.h"
#include "internal/Assertions.h"

namespace lab {

namespace {

// ChannelProvider provides a single channel of audio data (one channel at a time) for each channel
// of data provided to us in a multi-channel provider.

class ChannelProvider : public AudioSourceProvider {
public:
    ChannelProvider(AudioSourceProvider* multiChannelProvider, unsigned numberOfChannels)
        : m_multiChannelProvider(multiChannelProvider)
        , m_numberOfChannels(numberOfChannels)
        , m_currentChannel(0)
        , m_framesToProcess(0)
    {
    }

    // provideInput() will be called once for each channel, starting with the first channel.
    // Each time it's called, it will provide the next channel of data.
    virtual void provideInput(AudioBus* bus, size_t framesToProcess)
    {
        bool isBusGood = bus && bus->numberOfChannels() == 1;
        ASSERT(isBusGood);
        if (!isBusGood)
            return;

        // Get the data from the multi-channel provider when the first channel asks for it.
        // For subsequent channels, we can just dish out the channel data from that (stored in m_multiChannelBus).
        if (m_currentChannel != 0) {
            m_framesToProcess = framesToProcess;
            m_multiChannelBus = std::unique_ptr<AudioBus>(new AudioBus(m_numberOfChannels, framesToProcess));
            m_multiChannelProvider->provideInput(m_multiChannelBus.get(), framesToProcess);
        }

        // All channels must ask for the same amount. This should always be the case, but let's just make sure.
        bool isGood = m_multiChannelBus.get() && framesToProcess == m_framesToProcess;
        ASSERT(isGood);
        if (!isGood)
            return;

        // Copy the channel data from what we received from m_multiChannelProvider.
        ASSERT(static_cast<unsigned>(m_currentChannel) <= m_numberOfChannels);
        if (static_cast<unsigned>(m_currentChannel) < m_numberOfChannels) {
            memcpy(bus->channel(0)->mutableData(),
                   m_multiChannelBus->channel(m_currentChannel)->data(), sizeof(float) * framesToProcess);
            
            // increment channel
            ++m_currentChannel;
        }
    }

private:
    AudioSourceProvider* m_multiChannelProvider;
    std::unique_ptr<AudioBus> m_multiChannelBus;
    unsigned m_numberOfChannels;
    unsigned m_currentChannel;
    size_t m_framesToProcess; // Used to verify that all channels ask for the same amount.
};

} // namespace

MultiChannelResampler::MultiChannelResampler(double scaleFactor, unsigned numberOfChannels)
    : m_numberOfChannels(numberOfChannels)
{
    // Create each channel's resampler.
    for (unsigned channelIndex = 0; channelIndex < numberOfChannels; ++channelIndex)
        m_kernels.push_back(std::unique_ptr<SincResampler>(new SincResampler(scaleFactor)));
}

void MultiChannelResampler::process(ContextRenderLock&, AudioSourceProvider* provider, AudioBus* destination, size_t framesToProcess)
{
    // The provider can provide us with multi-channel audio data. But each of our single-channel resamplers (kernels)
    // below requires a provider which provides a single unique channel of data.
    // channelProvider wraps the original multi-channel provider and dishes out one channel at a time.
    ChannelProvider channelProvider(provider, m_numberOfChannels);

    for (unsigned channelIndex = 0; channelIndex < m_numberOfChannels; ++channelIndex) {
        // Depending on the sample-rate scale factor, and the internal buffering used in a SincResampler
        // kernel, this call to process() will only sometimes call provideInput() on the channelProvider.
        // However, if it calls provideInput() for the first channel, then it will call it for the remaining
        // channels, since they all buffer in the same way and are processing the same number of frames.
        m_kernels[channelIndex]->process(&channelProvider,
                                         destination->channel(channelIndex)->mutableData(),
                                         framesToProcess);
    }
}

} // namespace lab
