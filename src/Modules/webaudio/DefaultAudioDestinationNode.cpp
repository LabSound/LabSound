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

#include "LabSoundConfig.h"
#include "DefaultAudioDestinationNode.h"

namespace WebCore {
    
DefaultAudioDestinationNode::DefaultAudioDestinationNode(std::shared_ptr<AudioContext> c)
: AudioDestinationNode(c, AudioDestination::hardwareSampleRate())
{
}

DefaultAudioDestinationNode::~DefaultAudioDestinationNode()
{
    LOG("Destruct %p", this);
    uninitialize();
}

void DefaultAudioDestinationNode::initialize()
{
    if (isInitialized())
        return;

    float hardwareSampleRate = AudioDestination::hardwareSampleRate();
    
    LOG("Hardware Samplerate: %f\n", hardwareSampleRate);
    
    m_destination = std::move(std::unique_ptr<AudioDestination>(AudioDestination::create(*this, hardwareSampleRate)));
    
    AudioNode::initialize();
}

void DefaultAudioDestinationNode::uninitialize()
{
    if (!isInitialized())
        return;

    m_destination->stop();

    AudioNode::uninitialize();
}

void DefaultAudioDestinationNode::startRendering()
{
    ASSERT(isInitialized());
    if (isInitialized())
        m_destination->start();
}

} // namespace WebCore
