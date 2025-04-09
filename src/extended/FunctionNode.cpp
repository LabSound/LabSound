// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/FunctionNode.h"
#include "LabSound/extended/Registry.h"

using namespace std;
using namespace lab;

namespace lab
{

AudioNodeDescriptor * FunctionNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr, 1};
    return &d;
}

FunctionNode::FunctionNode(AudioContext & ac, int channels)
: AudioScheduledSourceNode(ac, {nullptr, nullptr, channels})
{
    _self->desiredChannelCount = channels;
    initialize();
}

FunctionNode::~FunctionNode()
{
    uninitialize();
}

void FunctionNode::process(ContextRenderLock & r, int bufferSize)
{
    auto outputBus = _self->output;

    if (!isInitialized() || !outputBus->numberOfChannels() || !_function)
    {
        outputBus->zero();
        return;
    }

    int quantumFrameOffset = _self->scheduler.renderOffset();
    int nonSilentFramesToProcess = _self->scheduler.renderLength();

    if (!nonSilentFramesToProcess)
    {
        outputBus->zero();
        return;
    }

    for (int i = 0; i < outputBus->numberOfChannels(); ++i)
    {
        float * destP = outputBus->channel(i)->mutableData();

        // Start rendering at the correct offset.
        destP += quantumFrameOffset;
        _function(r, this, static_cast<int>(i), destP, nonSilentFramesToProcess);
    }

    _now += double(bufferSize) / r.context()->sampleRate();
    outputBus->clearSilentFlag();
}

void FunctionNode::reset(ContextRenderLock & r)
{
    AudioNode::reset(r);
}

bool FunctionNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}

}  // namespace lab
