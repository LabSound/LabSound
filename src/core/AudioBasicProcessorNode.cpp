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

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"

#include "internal/Assertions.h"
#include "internal/AudioBus.h"

namespace WebCore {

AudioBasicProcessorNode::AudioBasicProcessorNode(float sampleRate) : AudioNode(sampleRate)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

    // The subclass must create m_processor.
}

void AudioBasicProcessorNode::initialize()
{
    if (isInitialized())
        return;

    ASSERT(processor());
    processor()->initialize();

    AudioNode::initialize();
}

void AudioBasicProcessorNode::uninitialize()
{
    if (!isInitialized())
        return;

    ASSERT(processor());
    processor()->uninitialize();

    AudioNode::uninitialize();
}

void AudioBasicProcessorNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    AudioBus* destinationBus = output(0)->bus(r);
    
    if (!isInitialized() || !processor() || processor()->numberOfChannels() != numberOfChannels())
        destinationBus->zero();
    else {
        AudioBus* sourceBus = input(0)->bus(r);

        // FIXME: if we take "tail time" into account, then we can avoid calling processor()->process() once the tail dies down.
        if (!input(0)->isConnected())
            sourceBus->zero();

        processor()->process(r, sourceBus, destinationBus, framesToProcess);
    }
}

// Nice optimization in the very common case allowing for "in-place" processing
void AudioBasicProcessorNode::pullInputs(ContextRenderLock& r, size_t framesToProcess)
{
    // Render input stream - suggest to the input to render directly into output bus for in-place processing in process() if possible.
    input(0)->pull(r, output(0)->bus(r), framesToProcess);
}

void AudioBasicProcessorNode::reset(std::shared_ptr<AudioContext>)
{
    if (processor())
        processor()->reset();
}

// As soon as we know the channel count of our input, we can lazily initialize.
// Sometimes this may be called more than once with different channel counts, in which case we must safely
// uninitialize and then re-initialize with the new channel count.
void AudioBasicProcessorNode::checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input)
{
    if (!input || input != this->input(0).get())
        return;

    if (!processor())
        return;

    unsigned numberOfChannels = input->numberOfChannels(r);
    
    if (isInitialized() && numberOfChannels != output(0)->numberOfChannels()) {
        uninitialize();
    }
    
    if (!isInitialized()) {
        // This will propagate the channel count to any nodes connected further down the chain...
        output(0)->setNumberOfChannels(r, numberOfChannels);

        // Re-initialize the processor with the new channel count.
        processor()->setNumberOfChannels(numberOfChannels);
        initialize();
    }

    AudioNode::checkNumberOfChannelsForInput(r, input);
}

unsigned AudioBasicProcessorNode::numberOfChannels()
{
    return output(0)->numberOfChannels();
}

double AudioBasicProcessorNode::tailTime() const
{
    return m_processor->tailTime();
}

double AudioBasicProcessorNode::latencyTime() const
{
    return m_processor->latencyTime();
}

AudioProcessor * AudioBasicProcessorNode::processor() 
{ 
	return m_processor.get(); 
}

} // namespace WebCore
