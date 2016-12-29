// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/GainNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioArray.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioBus.h"
#include "internal/Assertions.h"

namespace lab
{

GainNode::GainNode(float sampleRate)
    : AudioNode(sampleRate)
    , m_lastGain(1.0)
    , m_sampleAccurateGainValues(AudioNode::ProcessingSizeInFrames) // FIXME: can probably share temp buffer in context
{
    m_gain = std::make_shared<AudioParam>("gain", 1.0, 0.0, 10000.0);

    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));

    m_params.push_back(m_gain);

    setNodeType(NodeTypeGain);

    initialize();
}

GainNode::~GainNode()
{
    uninitialize();
}

void GainNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    // FIXME: for some cases there is a nice optimization to avoid processing here, and let the gain change
    // happen in the summing junction input of the AudioNode we're connected to.
    // Then we can avoid all of the following:

    AudioBus* outputBus = output(0)->bus(r);
    ASSERT(outputBus);

    if (!isInitialized() || !input(0)->isConnected())
        outputBus->zero();
    else {
        AudioBus* inputBus = input(0)->bus(r);

        if (gain()->hasSampleAccurateValues()) {
            // Apply sample-accurate gain scaling for precise envelopes, grain windows, etc.
            ASSERT(framesToProcess <= m_sampleAccurateGainValues.size());
            if (framesToProcess <= m_sampleAccurateGainValues.size()) {
                float* gainValues = m_sampleAccurateGainValues.data();
                gain()->calculateSampleAccurateValues(r, gainValues, framesToProcess);
                outputBus->copyWithSampleAccurateGainValuesFrom(*inputBus, gainValues, framesToProcess);
            }
        }
        else {
            // Apply the gain with de-zippering into the output bus.
            outputBus->copyWithGainFrom(*inputBus, &m_lastGain, gain()->value(r));
        }
    }
}

void GainNode::reset(ContextRenderLock& r)
{
    // Snap directly to desired gain.
    m_lastGain = gain()->value(r);
}

// FIXME: this can go away when we do mixing with gain directly in summing junction of AudioNodeInput
//
// As soon as we know the channel count of our input, we can lazily initialize.
// Sometimes this may be called more than once with different channel counts, in which case we must safely
// uninitialize and then re-initialize with the new channel count.
void GainNode::checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input)
{
    if (!input)
        return;

    ASSERT(r.context());

    if (input != this->input(0).get())
        return;

    unsigned numberOfChannels = input->numberOfChannels(r);

    if (isInitialized() && numberOfChannels != output(0)->numberOfChannels())
    {
        // We're already initialized but the channel count has changed.
        uninitialize();
    }

    if (!isInitialized())
    {
        // This will propagate the channel count to any nodes connected further downstream in the graph.
        output(0)->setNumberOfChannels(r, numberOfChannels);
        initialize();
    }

    AudioNode::checkNumberOfChannelsForInput(r, input);
}

} // namespace lab
