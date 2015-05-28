/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSound/core/DefaultAudioDestinationNode.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"

#include "internal/Assertions.h"
#include "internal/AudioDestination.h"

namespace WebCore {
    
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
    
} // namespace WebCore
