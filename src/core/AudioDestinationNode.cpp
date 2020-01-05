// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioDestinationNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSourceProvider.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/AudioUtilities.h"
#include "internal/DenormalDisabler.h"

#include <chrono>

namespace lab
{

// LocalAudioInputProvider allows us to expose an AudioSourceProvider for local/live audio input.
// If there is local/live audio input, we call copyBusData() with the audio input data every render quantum.
class AudioDestinationNode::LocalAudioInputProvider : public AudioSourceProvider
{
    AudioBus * m_sourceBus = nullptr;

public:
    LocalAudioInputProvider()
    {
        epoch[0] = epoch[1] = std::chrono::high_resolution_clock::now();
    }

    virtual ~LocalAudioInputProvider()
    {
        delete m_sourceBus;
    }

    void copyBusData(AudioBus * bus)
    {
        if (!bus)
            return;

        if (!m_sourceBus || bus->numberOfChannels() != m_sourceBus->numberOfChannels() || bus->length() != m_sourceBus->length())
        {
            if (m_sourceBus)
                delete m_sourceBus;
            m_sourceBus = new AudioBus(bus->numberOfChannels(), bus->length());
        }

        m_sourceBus->copyFrom(*bus);
    }

    // AudioSourceProvider.
    virtual void provideInput(AudioBus * destinationBus, size_t numberOfFrames) override
    {
        if (!m_sourceBus)
            return;

        bool isGood = destinationBus && destinationBus->length();
        ASSERT(isGood);
        if (isGood)
            destinationBus->copyFrom(*m_sourceBus);
    }

    virtual void provideInput(uint32_t channel, float * destination, size_t numberOfFrames) override
    {
        if (!m_sourceBus)
            return;

        bool isGood = destination && m_sourceBus && m_sourceBus->channel(channel) && m_sourceBus->channel(channel)->data();
        ASSERT(isGood);
        if (!isGood)
            return;

        size_t sz = numberOfFrames * sizeof(float);
        size_t src_sz = m_sourceBus->channel(channel)->size() * sizeof(float);
        sz = sz < src_sz ? sz : src_sz;
        if (sz)
            memcpy(destination, m_sourceBus->channel(channel)->data(), sz);
    }

    virtual uint32_t numberOfChannels() const override
    {
        return m_sourceBus ? m_sourceBus->numberOfChannels() : 0;
    }

    // Counts the number of sample-frames processed by the destination.
    uint64_t m_currentSampleFrame = 0;

    // low bit of m_currentSampleFrame indexes time point 0 or 1
    // this is so that time and epoch are written atomically, after the alternative epoch has been filled in.
    std::chrono::high_resolution_clock::time_point epoch[2];
};

AudioDestinationNode::AudioDestinationNode(AudioContext * context, uint32_t channelCount, float sampleRate)
    : m_sampleRate(sampleRate)
    , m_context(context)
{
    m_localAudioInputProvider = new LocalAudioInputProvider();

    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));

    // Node-specific default mixing rules.
    m_channelCount = channelCount;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Speakers;

    m_localAudioInputProvider->epoch[0] = m_localAudioInputProvider->epoch[1] = std::chrono::high_resolution_clock::now();

    // NB: Special case - the audio context calls initialize so that rendering doesn't start before the context is ready
    // initialize();
}

AudioDestinationNode::~AudioDestinationNode()
{
    uninitialize();
}

void AudioDestinationNode::reset(ContextRenderLock &)
{
    m_localAudioInputProvider->m_currentSampleFrame = 0;
};

void AudioDestinationNode::render(AudioBus * sourceBus, AudioBus * destinationBus, uint32_t numberOfFrames)
{
    // The audio system might still be invoking callbacks during shutdown, so bail out if so.
    if (!m_context)
        return;

    ContextRenderLock renderLock(m_context, "AudioDestinationNode::render");
    if (!renderLock.context())
        return;  // return if couldn't acquire lock

    m_context->setCurrentFrames(numberOfFrames);

    if (!m_context->isInitialized())
    {
        destinationBus->zero();
        return;
    }

    // Denormals can slow down audio processing.
    /// @TODO under what circumstance do they arise?
    /// If they come from input data such as loaded WAV files, they should be corrected
    /// at source. If they can result from signal processing; again, where? The
    /// signal processing should not produce denormalized values.
    // Use an RAII object to protect all AudioNodes processed within this scope.
    DenormalDisabler denormalDisabler;

    // Let the context take care of any business at the start of each render quantum.
    m_context->handlePreRenderTasks(renderLock);

    // Prepare the local audio input provider for this render quantum.
    if (sourceBus)
        m_localAudioInputProvider->copyBusData(sourceBus);

    /// @TODO why is only input 0 processed?

    // process the graph by pulling the inputs, which will recurse the entire processing graph.
    AudioBus * renderedBus = input(0)->pull(renderLock, destinationBus);

    if (!renderedBus)
    {
        destinationBus->zero();
    }
    else if (renderedBus != destinationBus)
    {
        // in-place processing was not possible - so copy
        destinationBus->copyFrom(*renderedBus);
    }

    // Process nodes which need a little extra help because they are not connected to anything, but still need to process.
    m_context->processAutomaticPullNodes(renderLock);

    // Let the context take care of any business at the end of each render quantum.
    m_context->handlePostRenderTasks(renderLock);

    // Advance current sample-frame.
    int index = 1 - (m_localAudioInputProvider->m_currentSampleFrame & 1);
    m_localAudioInputProvider->epoch[index] = std::chrono::high_resolution_clock::now();
    uint64_t t = m_localAudioInputProvider->m_currentSampleFrame & ~1;
    m_localAudioInputProvider->m_currentSampleFrame = t + numberOfFrames * 2 + index;
}

uint64_t AudioDestinationNode::currentSampleFrame() const
{
    return m_localAudioInputProvider->m_currentSampleFrame / 2;
}

double AudioDestinationNode::currentTime() const
{
    return currentSampleFrame() / static_cast<double>(m_sampleRate);
}

double AudioDestinationNode::currentSampleTime() const
{
    uint64_t t = m_localAudioInputProvider->m_currentSampleFrame;
    int index = t & 1;
    double val = (t / 2) / static_cast<double>(m_sampleRate);
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point::duration dt = t2 - m_localAudioInputProvider->epoch[index];
    return val + (std::chrono::duration_cast<std::chrono::microseconds>(dt).count() * 1.0e-6f);
}

AudioSourceProvider * AudioDestinationNode::localAudioInputProvider()
{
    return static_cast<AudioSourceProvider *>(m_localAudioInputProvider);
}

}  // namespace lab
