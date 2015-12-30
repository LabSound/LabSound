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

#include "LabSound/core/AudioHardwareSourceNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSourceProvider.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"

#include "internal/AudioBus.h"

namespace WebCore {

AudioHardwareSourceNode::AudioHardwareSourceNode(AudioSourceProvider * audioSourceProvider, float sampleRate) : AudioSourceNode(sampleRate)
, m_audioSourceProvider(audioSourceProvider)
, m_sourceNumberOfChannels(0)
{
    
    //@tofix: Default to stereo.
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 2)));

    setNodeType(LabSound::NodeType::NodeTypeHardwareSource);

    initialize();
}

AudioHardwareSourceNode::~AudioHardwareSourceNode()
{
    uninitialize();
}

void AudioHardwareSourceNode::setFormat(ContextRenderLock & r, size_t numberOfChannels, float sourceSampleRate)
{
    if (numberOfChannels != m_sourceNumberOfChannels || sourceSampleRate != sampleRate())
    {
        // The sample-rate must be equal to the context's sample-rate.
        if (!numberOfChannels || numberOfChannels > AudioContext::maxNumberOfChannels || sourceSampleRate != sampleRate())
            throw std::runtime_error("AudioHardwareSourceNode must match samplerate of context... ");

        m_sourceNumberOfChannels = numberOfChannels;
        
        // Do any necesssary re-configuration to the output's number of channels.
        output(0)->setNumberOfChannels(r, numberOfChannels);
    }
}

void AudioHardwareSourceNode::process(ContextRenderLock& r, size_t numberOfFrames)
{
    AudioBus * outputBus = output(0)->bus(r);

    if (!audioSourceProvider())
    {
        outputBus->zero();
        return;
    }

    if (m_sourceNumberOfChannels != outputBus->numberOfChannels())
    {
        outputBus->zero();
        return;
    }

    // Use a tryLock() to avoid contention in the real-time audio thread.
    // If we fail to acquire the lock then the MediaStream must be in the middle of
    // a format change, so we output silence in this case.
    if (r.context())
    {
        audioSourceProvider()->provideInput(outputBus, numberOfFrames);
    }
    else
    {
        // We failed to acquire the lock.
        outputBus->zero();
    }
}

void AudioHardwareSourceNode::reset(ContextRenderLock&)
{
    
}

} // namespace WebCore
