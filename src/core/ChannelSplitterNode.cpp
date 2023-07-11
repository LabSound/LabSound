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
    
    /// @TODO the ONLY reason there's multiple outputs in the webaudio spec at all is for this
    /// one node. Need to figure out how to satisfy a desire for a splitterNode for compatibility, and
    /// also not cause undue compexity to satisfy it.
    
    auto destination = output();
    if (destination)
        destination->channel(0)->copyFrom(source->channel(0));
}

}  // namespace lab
