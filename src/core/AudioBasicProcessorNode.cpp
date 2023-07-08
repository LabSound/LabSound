// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"

#include "internal/Assertions.h"

namespace lab
{

AudioBasicProcessorNode::AudioBasicProcessorNode(AudioContext & ac, AudioNodeDescriptor const& desc)
: AudioNode(ac, desc)
{
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
    AudioBus * destinationBus = _self->output.get();

    if (!isInitialized() || !processor()) {
        if (destinationBus)
            destinationBus->zero();
        return;
    }

    auto sourceBus = summedInput();
    if (!sourceBus) {
        destinationBus->zero();
        return;
    }

    // process entire buffer
    processor()->process(r, sourceBus.get(), destinationBus, bufferSize);
}


void AudioBasicProcessorNode::reset(ContextRenderLock& r)
{
    AudioNode::reset(r);
    if (processor())
        processor()->reset();
}

int AudioBasicProcessorNode::numberOfChannels()
{
    return _self->output->numberOfChannels();
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
