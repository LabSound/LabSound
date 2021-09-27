// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"

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
    addInput("carrier");
    addInput("modulator");
    addOutput("out", 1, AudioNode::ProcessingSizeInFrames);
    initialize();
}

PWMNode::~PWMNode()
{
    uninitialize();
}

void PWMNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * dstBus = outputBus(r, 0);
    AudioBus * carrierBus = inputBus(r, 0);
    AudioBus * modBus = inputBus(r, 1);

    if (!dstBus || !carrierBus)
    {
        if (dstBus)
            dstBus->zero();
        return;
    }
    if (!modBus) {
        dstBus->copyFrom(*carrierBus);
        return;
    }

    const float * modP = modBus->channel(0)->data();
    const float * carrierP = carrierBus->channel(0)->data();

    float * destP = dstBus->channel(0)->mutableData();
    int n = bufferSize;
    while (n--)
    {
        float carrier = *carrierP++;
        float mod = *modP++;
        *destP++ = (carrier > mod) ? 1.0f : -1.0f;
    }
}

} // lab
