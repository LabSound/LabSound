/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
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

#include "LabSound/core/MediaStreamAudioDestinationNode.h"

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/MediaStream.h"

#include "internal/AudioBus.h"

namespace WebCore {

MediaStreamAudioDestinationNode::MediaStreamAudioDestinationNode(size_t numberOfChannels, float sampleRate) : AudioBasicInspectorNode(sampleRate)
{
	m_mixBus = new AudioBus(numberOfChannels, ProcessingSizeInFrames);
    setNodeType(NodeTypeMediaStreamAudioDestination);

    initialize();
}

MediaStreamSource* MediaStreamAudioDestinationNode::mediaStreamSource()
{
    return m_source.get();
}

MediaStreamAudioDestinationNode::~MediaStreamAudioDestinationNode()
{
    uninitialize();
	delete m_mixBus; // Dimitri
}

//@tofix
void MediaStreamAudioDestinationNode::process(ContextRenderLock&, size_t numberOfFrames)
{
    m_mixBus->copyFrom(*input(0)->bus());
    
    // m_source is supposed to be derived from AudioDestinationConsumer.h
    // --- it should be very easy to pipe the audio from LabSound to something else via that API
    
	// LabSound commented - will need to revisit later
	// m_source->consumeAudio(&m_mixBus, numberOfFrames);
}

void MediaStreamAudioDestinationNode::reset(std::shared_ptr<AudioContext>)
{
}

} // namespace WebCore
