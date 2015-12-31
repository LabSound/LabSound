// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"

#include "internal/Assertions.h"
#include "internal/AudioBus.h"

namespace lab {

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

void AudioBasicProcessorNode::reset(ContextRenderLock&)
{
    if (processor())
        processor()->reset();
}

// As soon as we know the channel count of our input, we can lazily initialize.
// Sometimes this may be called more than once with different channel counts, in which case we must safely
// uninitialize and then re-initialize with the new channel count.
void AudioBasicProcessorNode::checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input)
{
    if (input != this->input(0).get())
        return;

    if (!processor())
        return;

    unsigned numberOfChannels = input->numberOfChannels(r);
    
    bool mustPropagate = false;
    for (size_t i = 0; i < numberOfOutputs() && !mustPropagate; ++i) {
        mustPropagate = isInitialized() && numberOfChannels != output(i)->numberOfChannels();
    }
    
    if (mustPropagate) {
        // Re-initialize the processor with the new channel count.
        processor()->setNumberOfChannels(numberOfChannels);
        
        uninitialize();
        for (size_t i = 0; i < numberOfOutputs(); ++i) {
            // This will propagate the channel count to any nodes connected further down the chain...
            output(i)->setNumberOfChannels(r, numberOfChannels);
        }
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

} // namespace lab
