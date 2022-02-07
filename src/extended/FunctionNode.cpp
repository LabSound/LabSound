// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/FunctionNode.h"
#include "LabSound/extended/Registry.h"

using namespace std;
using namespace lab;

namespace lab
{

AudioNodeDescriptor * FunctionNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr};
    return &d;
}

FunctionNode::FunctionNode(AudioContext & ac, int channels)
    : AudioScheduledSourceNode(ac, *desc())
{
    addOutput(channels, AudioNode::ProcessingSizeInFrames);
    initialize();
}

FunctionNode::~FunctionNode()
{
    uninitialize();
}

void FunctionNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * dstBus = outputBus(r);

    if (!isInitialized() || !dstBus->numberOfChannels() || !_function)
    {
        dstBus->zero();
        return;
    }

    int quantumFrameOffset = _scheduler._renderOffset;
    int nonSilentFramesToProcess = _scheduler._renderLength;

    if (!nonSilentFramesToProcess)
    {
        dstBus->zero();
        return;
    }

    for (int i = 0; i < dstBus->numberOfChannels(); ++i)
    {
        float * destP = dstBus->channel(i)->mutableData();

        // Start rendering at the correct offset.
        destP += quantumFrameOffset;
        _function(r, this, static_cast<int>(i), destP, nonSilentFramesToProcess);
    }

    _now += double(bufferSize) / r.context()->sampleRate();
    dstBus->clearSilentFlag();
}

void FunctionNode::reset(ContextRenderLock & r)
{
    // No-op
}

bool FunctionNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}

}  // namespace lab
