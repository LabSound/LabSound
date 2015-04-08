/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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

#ifndef AudioDestinationNode_h
#define AudioDestinationNode_h

#include "LabSound/core/AudioBuffer.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioIOCallback.h"


namespace WebCore {

class AudioBus;
class AudioContext;
class AudioSourceProvider;
class LocalAudioInputProvider;

class AudioDestinationNode : public AudioNode, public AudioIOCallback {

	class LocalAudioInputProvider;
	LocalAudioInputProvider * m_localAudioInputProvider;

public:

    AudioDestinationNode(std::shared_ptr<AudioContext>, float sampleRate);

    virtual ~AudioDestinationNode();
    
    // AudioNode   
    virtual void process(ContextRenderLock&, size_t) override { } // we're pulled by hardware so this is never called
    virtual void reset(std::shared_ptr<AudioContext>) override { m_currentSampleFrame = 0; };
    
    // The audio hardware calls render() to get the next render quantum of audio into destinationBus.
    // It will optionally give us local/live audio input in sourceBus (if it's not 0).
    virtual void render(AudioBus* sourceBus, AudioBus* destinationBus, size_t numberOfFrames) override;

    size_t currentSampleFrame() const { return m_currentSampleFrame; }
    double currentTime() const { return currentSampleFrame() / static_cast<double>(sampleRate()); }

    virtual unsigned numberOfChannels() const { return 2; } // FIXME: update when multi-channel (more than stereo) is supported

    virtual void startRendering() = 0;

    AudioSourceProvider * localAudioInputProvider();
    
protected:

    virtual double tailTime() const override { return 0; }
    virtual double latencyTime() const override { return 0; }

    // Counts the number of sample-frames processed by the destination.
    size_t m_currentSampleFrame;

    std::shared_ptr<AudioContext> m_context;

};

} // namespace WebCore

#endif // AudioDestinationNode_h
