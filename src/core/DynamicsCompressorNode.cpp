// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/DynamicsCompressorNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

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
    static AudioNodeDescriptor d {s_dcParams, nullptr, 0};
    return &d;
}

DynamicsCompressorNode::DynamicsCompressorNode(AudioContext& ac)
    : AudioNode(ac, *desc())
{
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
    /// @fixme it doesn't make sense for these parameters to be animated over time.
    /// @todo Switch them to settings
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

    int numberOfSourceChannels = _self->summingBus->numberOfChannels();
    auto outputBus = _self->output;
    int numberOfDestChannels = outputBus->numberOfChannels();
    if (numberOfDestChannels != numberOfSourceChannels)
    {
        outputBus->setNumberOfChannels(r, numberOfSourceChannels);
        numberOfDestChannels = output()->numberOfChannels();
    }

    m_dynamicsCompressor->process(r,
                                  _self->summingBus.get(), outputBus.get(),
                                  bufferSize,
                                  _self->scheduler.renderOffset(),
                                  _self->scheduler.renderLength());

    float reduction = m_dynamicsCompressor->parameterValue(DynamicsCompressor::ParamReduction);
    m_reduction->setValueAtTime(reduction, 0);
}

void DynamicsCompressorNode::reset(ContextRenderLock& r)
{
    AudioNode::reset(r);
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
