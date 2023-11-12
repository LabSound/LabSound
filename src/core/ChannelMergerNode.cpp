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
    addInputs(numberOfInputs_);
    initialize();
}

void ChannelMergerNode::addInputs(int n)
{
    // Create the requested number of inputs.
    for (int i = 0; i < n; ++i)
        addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
}

void ChannelMergerNode::process(ContextRenderLock & r, int bufferSize)
{
    auto output = this->output(0);
    ASSERT_UNUSED(bufferSize, bufferSize == output->bus(r)->length());

    if (m_desiredNumberOfOutputChannels != output->numberOfChannels())
    {
        output->setNumberOfChannels(r, m_desiredNumberOfOutputChannels);
    }

    // Merge all the the zero-etch channels from all the inputs into one output.
    /* Per the spec, a channel merge node takes a bunch of mono inputs.
    The processing should be as follows

    let a = number of input channels
    let b = number of output channels

    if a == b copy 1:1
    if a < b copy 1:1 up to a, then zero the other channels
    if b < a copy 1:1 up to b, then ignore the other channels
    */

    for (int i = 0; i < m_desiredNumberOfOutputChannels; ++i)
    {
        // initialize sum
        output->bus(r)->channel(i)->zero();
    }

    int in = numberOfInputs();
    for (int c = 0; c < m_desiredNumberOfOutputChannels; ++c)
    {
        AudioChannel * outputChannel = output->bus(r)->channel(c);
        if (c < in) 
        {
            auto input = this->input(c);
            if (input->isConnected())
            {
                AudioChannel * inputChannel = input->bus(r)->channel(0);
                outputChannel->sumFrom(inputChannel);
            }
        }
    }
}


}  // namespace lab
