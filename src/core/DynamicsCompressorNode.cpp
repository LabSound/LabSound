// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/DynamicsCompressorNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/DynamicsCompressor.h"
#include "internal/Assertions.h"

using namespace std;

namespace lab
{

DynamicsCompressorNode::DynamicsCompressorNode(float sampleRate) : AudioNode(sampleRate)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 2)));

    setNodeType(NodeTypeDynamicsCompressor);

    m_threshold = make_shared<AudioParam>("threshold", -24, -100, 0);
    m_knee = make_shared<AudioParam>("knee", 30, 0, 40);
    m_ratio = make_shared<AudioParam>("ratio", 12, 1, 20);
    m_reduction = make_shared<AudioParam>("reduction", 0, -20, 0);
    m_attack = make_shared<AudioParam>("attack", 0.003, 0, 1);
    m_release = make_shared<AudioParam>("release", 0.250, 0, 1);

    m_params.push_back(m_threshold);
    m_params.push_back(m_knee);
    m_params.push_back(m_ratio);
    m_params.push_back(m_reduction);
    m_params.push_back(m_attack);
    m_params.push_back(m_release);

    initialize();
}

DynamicsCompressorNode::~DynamicsCompressorNode()
{
    uninitialize();
}

void DynamicsCompressorNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    AudioBus* outputBus = output(0)->bus(r);
    ASSERT(outputBus);

    float threshold = m_threshold->value(r);
    float knee = m_knee->value(r);
    float ratio = m_ratio->value(r);
    float attack = m_attack->value(r);
    float release = m_release->value(r);

    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamThreshold, threshold);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamKnee, knee);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamRatio, ratio);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamAttack, attack);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamRelease, release);

    m_dynamicsCompressor->process(r, input(0)->bus(r), outputBus, framesToProcess);

    float reduction = m_dynamicsCompressor->parameterValue(DynamicsCompressor::ParamReduction);
    m_reduction->setValue(reduction);
}

void DynamicsCompressorNode::reset(ContextRenderLock&)
{
    m_dynamicsCompressor->reset();
}

void DynamicsCompressorNode::initialize()
{
    if (isInitialized())
        return;

    m_dynamicsCompressor.reset(new DynamicsCompressor(sampleRate(), 2));

    AudioNode::initialize();
}

void DynamicsCompressorNode::uninitialize()
{
    if (!isInitialized())
        return;

    AudioNode::uninitialize();

    m_dynamicsCompressor.reset();
}

double DynamicsCompressorNode::tailTime() const
{
    return m_dynamicsCompressor->tailTime();
}

double DynamicsCompressorNode::latencyTime() const
{
    return m_dynamicsCompressor->latencyTime();
}

} // end namespace lab
