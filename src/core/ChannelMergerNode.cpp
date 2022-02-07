// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/ChannelMergerNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"

using namespace std;

namespace lab
{

AudioNodeDescriptor * ChannelMergerNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr};
    return &d;
}

ChannelMergerNode::ChannelMergerNode(AudioContext & ac, int numberOfInputs_)
    : AudioNode(ac, *desc())
    , m_desiredNumberOfOutputChannels(1)
{
    addInputs(numberOfInputs_);
    addOutput(1, AudioNode::ProcessingSizeInFrames);
    initialize();
}

void ChannelMergerNode::addInputs(int n)
{
    // Create the requested number of inputs.
    for (int i = 0; i < n; ++i)
        addInput("in");
}

void ChannelMergerNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * dstBus = outputBus(r);
    ASSERT_UNUSED(bufferSize, bufferSize == dstBus->length());

    if (m_desiredNumberOfOutputChannels != dstBus->numberOfChannels())
    {
        dstBus->setNumberOfChannels(r, m_desiredNumberOfOutputChannels);
    }

    // Merge all the channels from all the inputs into one output.
    uint32_t outputChannelIndex = 0;
    for (int i = 0; i < numberOfInputs(); ++i)
    {
        auto input = inputBus(r, i);

        if (input)
        {
            size_t numberOfInputChannels = input->numberOfChannels();

            // Merge channels from this particular input.
            for (int j = 0; j < numberOfInputChannels; ++j)
            {
                AudioChannel * inputChannel = input->channel(j);
                AudioChannel * outputChannel = dstBus->channel(outputChannelIndex);

                outputChannel->copyFrom(inputChannel);
                ++outputChannelIndex;
            }
        }
    }
}


}  // namespace lab
