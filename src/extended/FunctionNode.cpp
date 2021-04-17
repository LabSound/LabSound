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

FunctionNode::FunctionNode(AudioContext & ac, int channels)
    : AudioScheduledSourceNode(ac)
{
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, channels)));

    if (s_registered)
       initialize();
}

bool FunctionNode::s_registered = NodeRegistry::Register(FunctionNode::static_name(),
    [](AudioContext& ac)->AudioNode* { return new FunctionNode(ac); },
    [](AudioNode* n) { delete n; });


FunctionNode::~FunctionNode()
{
    uninitialize();
}

void FunctionNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * outputBus = output(0)->bus(r);

    if (!isInitialized() || !outputBus->numberOfChannels() || !_function)
    {
        outputBus->zero();
        return;
    }

    int quantumFrameOffset = _scheduler._renderOffset;
    int nonSilentFramesToProcess = _scheduler._renderLength;

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
    // No-op
}

bool FunctionNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}

}  // namespace lab
