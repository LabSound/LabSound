// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/ChannelMergerNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"

using namespace std;

namespace lab
{

AudioNodeDescriptor * ChannelMergerNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr, 0};
    return &d;
}

ChannelMergerNode::ChannelMergerNode(AudioContext & ac, int numberOfInputs_)
    : AudioNode(ac, *desc())
    , m_desiredNumberOfOutputChannels(numberOfInputs_)
{
    initialize();
}


void ChannelMergerNode::process(ContextRenderLock & r, int bufferSize)
{
    auto inputBus = summedInput();
    auto output = _self->output.get();
    
    if (!output) {
        _self->output.reset(new AudioBus(m_desiredNumberOfOutputChannels, bufferSize));
    }

    if (m_desiredNumberOfOutputChannels != output->numberOfChannels())
    {
        output->setNumberOfChannels(r, m_desiredNumberOfOutputChannels);
    }

    for (int i = 0; i < m_desiredNumberOfOutputChannels; ++i)
    {
        // initialize output sum to zero
        output->channel(i)->zero();
    }

    int in = numberOfInputs();
    for (int c = 0; c < m_desiredNumberOfOutputChannels; ++c)
    {
        AudioChannel * outputChannel = output->channel(c);
        if (c < in) 
        {
            auto& input = _self->inputs[c];
            AudioChannel * inputChannel = input.node->output()->channel(0);
            outputChannel->sumFrom(inputChannel);
        }
    }
}


}  // namespace lab
