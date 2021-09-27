// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/ChannelSplitterNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
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
    addInput("in");
    addOutputs(numberOfOutputs_);
    initialize();
}

void ChannelSplitterNode::addOutputs(int numberOfOutputs_)
{
    // Create a fixed number of outputs (able to handle the maximum number of channels fed to an input).
    for (int i = 0; i < numberOfOutputs_; ++i)
    {
        std::string buff("out");
        buff += std::to_string(i);
        addOutput(buff, 1, AudioNode::ProcessingSizeInFrames);
    }
}

void ChannelSplitterNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * source = inputBus(r, 0);
    ASSERT(source);

    size_t numberOfSourceChannels = source->numberOfChannels();

    for (int i = 0; i < numberOfOutputs(); ++i)
    {
        AudioBus * destination = outputBus(r, i);
        if (!destination)
            continue;
        ASSERT(destination);

        if (i < numberOfSourceChannels)
        {
            // Split the channel out if it exists in the source.
            // It would be nice to avoid the copy and simply pass along pointers, but this becomes extremely difficult with fanout and fanin.
            destination->channel(0)->copyFrom(source->channel(i));
        }
        else
        {
            destination->zero();
        }
    }
}

void ChannelSplitterNode::reset(ContextRenderLock &)
{
}

}  // namespace lab
