// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/DefaultAudioDestinationNode.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"

#include "internal/Assertions.h"
#include "internal/AudioDestination.h"

namespace lab {
    
DefaultAudioDestinationNode::DefaultAudioDestinationNode(std::shared_ptr<AudioContext> c)
    : AudioDestinationNode(c, AudioDestination::hardwareSampleRate())
{
    // Node-specific default mixing rules.
    m_channelCount = 2;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Speakers;
}

DefaultAudioDestinationNode::~DefaultAudioDestinationNode()
{
    uninitialize();
}

void DefaultAudioDestinationNode::initialize()
{
    if (isInitialized())
        return;

    createDestination();
    AudioNode::initialize();
}

void DefaultAudioDestinationNode::uninitialize()
{
    if (!isInitialized())
        return;

    m_destination->stop();
    AudioNode::uninitialize();
}

    
void DefaultAudioDestinationNode::createDestination()
{
    float hardwareSampleRate = AudioDestination::hardwareSampleRate();
    LOG("Hardware Samplerate: %f", hardwareSampleRate);
    m_destination = std::unique_ptr<AudioDestination>(AudioDestination::MakePlatformAudioDestination(*this, channelCount(), hardwareSampleRate));
}

void DefaultAudioDestinationNode::startRendering()
{
    ASSERT(isInitialized());
    if (isInitialized())
        m_destination->start();
}
    
unsigned DefaultAudioDestinationNode::maxChannelCount() const
{
    return AudioDestination::maxChannelCount();
}

void DefaultAudioDestinationNode::setChannelCount(ContextGraphLock& g, unsigned long channelCount)
{
    // The channelCount for the input to this node controls the actual number of channels we
    // send to the audio hardware. It can only be set depending on the maximum number of
    // channels supported by the hardware.
    
    ASSERT(g.context());
    
    if (!maxChannelCount() || channelCount > maxChannelCount())
    {
        throw std::invalid_argument("Max channel count invalid");
    }
    
    unsigned long oldChannelCount = this->channelCount();
    AudioNode::setChannelCount(g, channelCount);
    
    if (this->channelCount() != oldChannelCount && isInitialized())
    {
        // Re-create destination.
        m_destination->stop();
        createDestination();
        m_destination->start();
    }
}
    
} // namespace lab
