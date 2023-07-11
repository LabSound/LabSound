// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/GainNode.h"
#include "LabSound/core/AudioArray.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"

namespace lab
{

static AudioParamDescriptor s_gainParams[] = {{"gain", "GAIN", 1.0, 0.0, 10000.0}, nullptr};
AudioNodeDescriptor * GainNode::desc()
{
    static AudioNodeDescriptor d {s_gainParams, nullptr};
    return &d;
}

GainNode::GainNode(AudioContext& ac)
: AudioNode(ac, *desc())
{
    m_gain = param("gain");
    initialize();
}

GainNode::~GainNode()
{
    uninitialize();
}

void GainNode::process(ContextRenderLock &r, int bufferSize)
{
    // FIXME: for some cases there is a nice optimization to avoid processing here, and let the gain change
    // happen in the summing junction input of the AudioNode we're connected to.
    // Then we can avoid all of the following:

    AudioBus * outputBus = _self->output.get();
    ASSERT(outputBus);


    if (!isInitialized() || !input(0)->isConnected())
    {
        outputBus->zero();
        return;
    }


    auto inputBus = _self->inputs[0].node->output();
    const int inputBusChannelCount = inputBus->numberOfChannels();
    if (!inputBusChannelCount)
    {
        outputBus->zero();
        return;
    }

    int outputBusChannelCount = outputBus->numberOfChannels();
    if (inputBusChannelCount != outputBusChannelCount)
    {
        output()->setNumberOfChannels(r, inputBusChannelCount);
        outputBusChannelCount = inputBusChannelCount;
        outputBus = _self->output.get();
    }

    const float* gainValues = gain()->bus()->channel(0)->data();
    outputBus->copyWithSampleAccurateGainValuesFrom(*inputBus, gainValues, bufferSize);
    outputBus->clearSilentFlag();
}

void GainNode::reset(ContextRenderLock & r)
{
    AudioNode::reset(r);
}


}  // namespace lab
