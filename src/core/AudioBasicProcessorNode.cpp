// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioProcessor.h"

#include "internal/Assertions.h"

namespace lab
{

AudioBasicProcessorNode::AudioBasicProcessorNode(AudioContext & ac, AudioNodeDescriptor const& desc)
    : AudioNode(ac, desc)
{
    addInput("in");
    addOutput("out", 1, AudioNode::ProcessingSizeInFrames);

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

void AudioBasicProcessorNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * destinationBus = outputBus(r, 0);
    AudioBus * sourceBus = inputBus(r, 0);
    if (!isInitialized() || !processor() || !sourceBus)
    {
        if (destinationBus)
            destinationBus->zero();
        return;
    }

    int srcChannelCount = sourceBus->numberOfChannels();
    int dstChannelCount = destinationBus->numberOfChannels();
    if (srcChannelCount != dstChannelCount)
    {
        destinationBus->setNumberOfChannels(r, srcChannelCount);
    }

    // process entire buffer
    processor()->process(r, sourceBus, destinationBus, bufferSize);
}


void AudioBasicProcessorNode::reset(ContextRenderLock &)
{
    if (processor())
        processor()->reset();
}


double AudioBasicProcessorNode::tailTime(ContextRenderLock & r) const
{
    return m_processor->tailTime(r);
}

double AudioBasicProcessorNode::latencyTime(ContextRenderLock & r) const
{
    return m_processor->latencyTime(r);
}

AudioProcessor * AudioBasicProcessorNode::processor()
{
    return m_processor.get();
}

AudioProcessor * AudioBasicProcessorNode::processor() const
{
    return m_processor.get();
}

}  // namespace lab
