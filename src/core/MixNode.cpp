// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/MixNode.h"
#include "LabSound/core/AudioArray.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"

#include <cstring>

namespace lab
{

static AudioParamDescriptor s_mixParams[] = {{"mix", "MIX", 0.0, 0.0, 1.0}, nullptr};

AudioNodeDescriptor* MixNode::desc()
{
    static AudioNodeDescriptor d {s_mixParams, nullptr, 1};
    return &d;
}

MixNode::MixNode(AudioContext& ac)
    : AudioNode(ac, *desc())
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));

    m_mix = param("mix");

    initialize();
}

MixNode::~MixNode()
{
    uninitialize();
}

void MixNode::process(ContextRenderLock& r, int bufferSize)
{
    AudioBus * outputBus = output(0)->bus(r);
    ASSERT(outputBus);

    if (!isInitialized() || !input(0)->isConnected() || !input(1)->isConnected())
    {
        outputBus->zero();
        return;
    }

    AudioBus * inputBusA = input(0)->bus(r);
    AudioBus * inputBusB = input(1)->bus(r);

    if (!inputBusA || !inputBusB)
    {
        outputBus->zero();
        return;
    }

    float mixValue = 0.0f;
    m_mix->calculateSampleAccurateValues(r, &mixValue, 1);

    int numberOfChannels = std::min(inputBusA->numberOfChannels(), inputBusB->numberOfChannels());
    if (outputBus->numberOfChannels() != numberOfChannels)
    {
        output(0)->setNumberOfChannels(r, numberOfChannels);
        outputBus = output(0)->bus(r);
    }

    numberOfChannels = std::min(numberOfChannels, outputBus->numberOfChannels());

    if (mixValue == 0.0f)
    {
        for (int i = 0; i < numberOfChannels; ++i)
        {
            const float* sourceData = inputBusA->channel(i)->data();
            float* destData = outputBus->channel(i)->mutableData();
            std::memcpy(destData, sourceData, sizeof(float) * bufferSize);
        }
    }
    else if (mixValue == 1.0f)
    {
        for (int i = 0; i < numberOfChannels; ++i)
        {
            const float* sourceData = inputBusB->channel(i)->data();
            float* destData = outputBus->channel(i)->mutableData();
            std::memcpy(destData, sourceData, sizeof(float) * bufferSize);
        }
    }
    else
    {
        float gainA = 1.0f - mixValue;
        float gainB = mixValue;
        for (int i = 0; i < numberOfChannels; ++i)
        {
            const float* sourceDataA = inputBusA->channel(i)->data();
            const float* sourceDataB = inputBusB->channel(i)->data();
            float* destData = outputBus->channel(i)->mutableData();
            for (int j = 0; j < bufferSize; ++j)
            {
                destData[j] = sourceDataA[j] * gainA + sourceDataB[j] * gainB;
            }
        }
    }

    outputBus->clearSilentFlag();
}

void MixNode::reset(ContextRenderLock& r)
{

}

}  // namespace lab