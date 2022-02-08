// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/DynamicsCompressorNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Registry.h"

#include "internal/Assertions.h"
#include "internal/DynamicsCompressor.h"

using namespace std;

namespace lab
{

static AudioParamDescriptor s_dcParams[] = {
    {"threshold", "THRS", -24,  -100,  0},
    {"knee",      "KNEE",  30,     0, 40},
    {"ratio",     "RATE",  12,     1,  20},
    {"reduction", "REDC",   0,   -20,  0},
    {"attack",    "ATCK",   0.003, 0,  1},
    {"release",   "RELS",   0.250, 0,  1},
    nullptr};

AudioNodeDescriptor * DynamicsCompressorNode::desc()
{
    static AudioNodeDescriptor d {s_dcParams, nullptr};
    return &d;
}

DynamicsCompressorNode::DynamicsCompressorNode(AudioContext& ac)
    : AudioNode(ac, *desc())
{
    addInput("in");
    _channelCount = 1;

    m_threshold = param("threshold");
    m_knee = param("knee");
    m_ratio = param("ratio");
    m_reduction = param("reduction");
    m_attack = param("attack");
    m_release = param("release");

    initialize();
}

DynamicsCompressorNode::~DynamicsCompressorNode()
{
    uninitialize();
}

void DynamicsCompressorNode::process(ContextRenderLock &r, int bufferSize)
{
    AudioBus * dstBus = outputBus(r);
    AudioBus * srcBus = inputBus(r, 0);

    if (!dstBus || !srcBus)
    {
        if (dstBus)
            dstBus->zero();
        return;
    }

    /// @fixme these values should be per sample, not per quantum
    /// -or- they should be settings if they don't vary per sample
    float threshold = m_threshold->value();
    float knee = m_knee->value();
    float ratio = m_ratio->value();
    float attack = m_attack->value();
    float release = m_release->value();

    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamThreshold, threshold);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamKnee, knee);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamRatio, ratio);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamAttack, attack);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamRelease, release);

    int numberOfSourceChannels = srcBus->numberOfChannels();

    int numberOfDestChannels = dstBus->numberOfChannels();
    if (numberOfDestChannels != srcBus->numberOfChannels())
    {
        dstBus->setNumberOfChannels(r, srcBus->numberOfChannels());
    }

    ASSERT(dstBus && dstBus->numberOfChannels() == numberOfSourceChannels);

    m_dynamicsCompressor->process(r, srcBus, dstBus, bufferSize, _scheduler._renderOffset, _scheduler._renderLength);

    float reduction = m_dynamicsCompressor->parameterValue(DynamicsCompressor::ParamReduction);
    m_reduction->setValue(reduction);

    dstBus->clearSilentFlag();
}

void DynamicsCompressorNode::reset(ContextRenderLock &)
{
    m_dynamicsCompressor->reset();
}

void DynamicsCompressorNode::initialize()
{
    if (isInitialized())
        return;

    m_dynamicsCompressor.reset(new DynamicsCompressor(2));

    AudioNode::initialize();
}

void DynamicsCompressorNode::uninitialize()
{
    if (!isInitialized())
        return;

    AudioNode::uninitialize();

    m_dynamicsCompressor.reset();
}

double DynamicsCompressorNode::tailTime(ContextRenderLock & r) const
{
    return m_dynamicsCompressor->tailTime(r);
}

double DynamicsCompressorNode::latencyTime(ContextRenderLock & r) const
{
    return m_dynamicsCompressor->latencyTime(r);
}

}  // end namespace lab
