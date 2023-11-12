// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/ConstantSourceNode.h"
#include "LabSound/core/AudioArray.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"

namespace lab
{

static AudioParamDescriptor s_constantFilterParams[] = {
    {"offset", "OFFSET", 1, 0.0, 20000.0},
    nullptr};
AudioNodeDescriptor * ConstantSourceNode::desc()
{
    static AudioNodeDescriptor d {s_constantFilterParams, nullptr, 1};
    return &d;
}

ConstantSourceNode::ConstantSourceNode(AudioContext & ac)
: AudioScheduledSourceNode(ac, *desc())
, m_sampleAccurateOffsetValues(AudioNode::ProcessingSizeInFrames)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    m_offset = param("offset");
    m_offset->setValue(1.0f);
    initialize();
}

ConstantSourceNode::~ConstantSourceNode()
{
    uninitialize();
}

bool ConstantSourceNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}

void ConstantSourceNode::process(ContextRenderLock & r, int bufferSize)
{
    return process_internal(r, bufferSize, _self->_scheduler._renderOffset, _self->_scheduler._renderLength);
}

void ConstantSourceNode::process_internal(ContextRenderLock & r, int bufferSize, int offset, int count)
{
    AudioBus * outputBus = output(0)->bus(r);
    ASSERT(outputBus);

    int nonSilentFramesToProcess = count;
    if (!nonSilentFramesToProcess)
    {
        outputBus->zero();
        return;
    }

    if (!isInitialized())
    {
        outputBus->zero();
        return;
    }

    int outputBusChannelCount = outputBus->numberOfChannels();

    if (bufferSize > m_sampleAccurateOffsetValues.size())
    {
        m_sampleAccurateOffsetValues.allocate(bufferSize);
    }

    // fetch the constants
    float * offsets = m_sampleAccurateOffsetValues.data();
    if (m_offset->hasSampleAccurateValues())
    {
        m_offset->calculateSampleAccurateValues(r, offsets, bufferSize);
    }
    else
    {
        m_offset->smooth(r);
        float val = m_offset->smoothedValue();
        for (int i = 0; i < bufferSize; ++i) offsets[i] = val;
    }

    for (int c = 0; c < outputBus->numberOfChannels(); c++)
    {
        float * destination = outputBus->channel(c)->mutableData() + offset;
        const double sample_rate = (double) r.context()->sampleRate();

        for (int i = offset; i < offset + nonSilentFramesToProcess; ++i)
        {
            destination[i] = offsets[i];
        }
    }

    outputBus->clearSilentFlag();
}

}  // namespace lab
