// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/ChannelSplitterNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"

namespace lab
{

AudioNodeDescriptor * ChannelSplitterNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr};
    return &d;
}

ChannelSplitterNode::ChannelSplitterNode(AudioContext & ac, int numberOfOutputs_)
: AudioNode(ac, *desc())
{
    initialize();
}

void ChannelSplitterNode::process(ContextRenderLock & r, int bufferSize)
{
    auto source = summedInput();
    if (!source) {
        _self->output->zero();
        return;
    }

    size_t numberOfSourceChannels = source->numberOfChannels();
    for (int i = 0; i < numberOfOutputs(); ++i)
    {
        AudioBus * destination = output(i)->bus(r);
        ASSERT(destination);

        if (i < numberOfSourceChannels)
        {
            // Split the channel out if it exists in the source.
            // It would be nice to avoid the copy and simply pass along pointers, but this becomes extremely difficult with fanout and fanin.
            destination->channel(0)->copyFrom(source->channel(i));
        }
        else if (output(i)->renderingFanOutCount() > 0)
        {
            // Only bother zeroing out the destination if it's connected to anything
            destination->zero();
        }
    }
}

void ChannelSplitterNode::reset(ContextRenderLock& r)
{
    AudioNode::reset(r);
}

}  // namespace lab
