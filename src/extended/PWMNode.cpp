// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/PWMNode.h"
#include "LabSound/extended/Registry.h"

#include <iostream>

using namespace lab;

namespace lab
{


AudioNodeDescriptor * PWMNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr};
    return &d;
}

PWMNode::PWMNode(AudioContext & ac)
    : lab::AudioNode(ac, *desc())
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));
    initialize();
}

PWMNode::~PWMNode()
{
    uninitialize();
}

void PWMNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * outputBus = output(0)->bus(r);
    if (!outputBus || !isInitialized() || !input(0)->isConnected())
    {
        if (outputBus)
            outputBus->zero();
        return;
    }

    AudioBus * carrierBus = input(0)->bus(r);
    if (!input(1) || !input(1)->isConnected())
    {
        outputBus->copyFrom(*carrierBus);
        return;
    }

    AudioBus * modBus = input(0)->bus(r);
    const float * modP = modBus->channel(0)->data();
    const float * carrierP = carrierBus->channel(0)->data();

    float * destP = outputBus->channel(0)->mutableData();
    int n = bufferSize;
    while (n--)
    {
        float carrier = *carrierP++;
        float mod = *modP++;
        *destP++ = (carrier > mod) ? 1.0f : -1.0f;
    }
}

} // lab
