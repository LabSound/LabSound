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

#include "LabSoundConfig.h"
#include "ConvolverNode.h"

#include "AudioBuffer.h"
#include "AudioContext.h"
#include "AudioContextLock.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "Reverb.h"

using namespace std;

// Note about empirical tuning:
// The maximum FFT size affects reverb performance and accuracy.
// If the reverb is single-threaded and processes entirely in the real-time audio thread,
// it's important not to make this too high.  In this case 8192 is a good value.
// But, the Reverb object is multi-threaded, so we want this as high as possible without losing too much accuracy.
// Very large FFTs will have worse phase errors. Given these constraints 32768 is a good compromise.
const size_t MaxFFTSize = 32768;

namespace WebCore {

ConvolverNode::ConvolverNode(float sampleRate)
    : AudioNode(sampleRate)
    , m_swapOnRender(false)
    , m_normalize(true)
{
    addInput(unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 2)));
    
    setNodeType(NodeTypeConvolver);
    
    initialize();
}

ConvolverNode::~ConvolverNode()
{
    uninitialize();
}

void ConvolverNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    if (m_swapOnRender) {
        m_reverb = std::move(m_newReverb);
        m_buffer = m_newBuffer;
        m_newBuffer.reset();
        m_swapOnRender = false;
    }
    
    AudioBus* outputBus = output(0)->bus();
    
    if (!isInitialized() || !m_reverb) {
        if (outputBus)
            outputBus->zero();
        return;
    }

    // Process using the convolution engine.
    // Note that we can handle the case where nothing is connected to the input, in which case we'll just feed silence into the convolver.
    // FIXME:  If we wanted to get fancy we could try to factor in the 'tail time' and stop processing once the tail dies down if
    // we keep getting fed silence.
    m_reverb->process(r, input(0)->bus(), outputBus, framesToProcess);
}

void ConvolverNode::reset(std::shared_ptr<AudioContext>)
{
    m_newReverb.reset();
    m_swapOnRender = true;
}

void ConvolverNode::initialize()
{
    if (isInitialized())
        return;
        
    AudioNode::initialize();
}

void ConvolverNode::uninitialize()
{
    if (!isInitialized())
        return;

    m_reverb.reset();
    AudioNode::uninitialize();
}

void ConvolverNode::setBuffer(std::shared_ptr<AudioBuffer> buffer)
{
    if (!buffer)
        return;

    unsigned numberOfChannels = buffer->numberOfChannels();
    size_t bufferLength = buffer->length();

    // The current implementation supports up to four channel impulse responses, which are interpreted as true-stereo (see Reverb class).
    bool isBufferGood = numberOfChannels > 0 && numberOfChannels <= 4 && bufferLength;
    ASSERT(isBufferGood);
    if (!isBufferGood)
        return;

    // Wrap the AudioBuffer by an AudioBus. It's an efficient pointer set and not a memcpy().
    // This memory is simply used in the Reverb constructor and no reference to it is kept for later use in that class.
    AudioBus bufferBus(numberOfChannels, bufferLength, false);
    for (unsigned i = 0; i < numberOfChannels; ++i)
        bufferBus.setChannelMemory(i, buffer->getChannelData(i)->data(), bufferLength);

    bufferBus.setSampleRate(buffer->sampleRate());

    // Create the reverb with the given impulse response.
    const bool nonRealtimeForLargeBuffers = false;
    m_newReverb = std::unique_ptr<Reverb>(new Reverb(&bufferBus, AudioNode::ProcessingSizeInFrames, MaxFFTSize, 2,
                                                  nonRealtimeForLargeBuffers, m_normalize));
    m_newBuffer = buffer;
    m_swapOnRender = true;
}

std::shared_ptr<AudioBuffer> ConvolverNode::buffer()
{
    if (m_swapOnRender) {
        return m_newBuffer;
    }
    return m_buffer;
}

double ConvolverNode::tailTime() const
{
    return m_reverb ? m_reverb->impulseResponseLength() / static_cast<double>(sampleRate()) : 0;
}

double ConvolverNode::latencyTime() const
{
    return m_reverb ? m_reverb->latencyFrames() / static_cast<double>(sampleRate()) : 0;
}

} // namespace WebCore
