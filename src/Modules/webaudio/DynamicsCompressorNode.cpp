/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSoundConfig.h"
#include "DynamicsCompressorNode.h"

#include "AudioContext.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "DynamicsCompressor.h"
#include "AudioContextLock.h"

// Set output to stereo by default.
static const unsigned defaultNumberOfOutputChannels = 2;

using namespace std;

namespace WebCore {

DynamicsCompressorNode::DynamicsCompressorNode(float sampleRate)
    : AudioNode(sampleRate)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, defaultNumberOfOutputChannels)));

    setNodeType(NodeTypeDynamicsCompressor);

    m_threshold = make_shared<AudioParam>("threshold", -24, -100, 0);
    m_knee = make_shared<AudioParam>("knee", 30, 0, 40);
    m_ratio = make_shared<AudioParam>("ratio", 12, 1, 20);
    m_reduction = make_shared<AudioParam>("reduction", 0, -20, 0);
    m_attack = make_shared<AudioParam>("attack", 0.003, 0, 1);
    m_release = make_shared<AudioParam>("release", 0.250, 0, 1);

    initialize();
}

DynamicsCompressorNode::~DynamicsCompressorNode()
{
    uninitialize();
}

void DynamicsCompressorNode::process(ContextGraphLock& g, ContextRenderLock& r, size_t framesToProcess)
{
    AudioBus* outputBus = output(0)->bus();
    ASSERT(outputBus);

    float threshold = m_threshold->value(r.contextPtr());
    float knee = m_knee->value(r.contextPtr());
    float ratio = m_ratio->value(r.contextPtr());
    float attack = m_attack->value(r.contextPtr());
    float release = m_release->value(r.contextPtr());

    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamThreshold, threshold);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamKnee, knee);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamRatio, ratio);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamAttack, attack);
    m_dynamicsCompressor->setParameterValue(DynamicsCompressor::ParamRelease, release);

    m_dynamicsCompressor->process(g, r, input(0)->bus(), outputBus, framesToProcess);

    float reduction = m_dynamicsCompressor->parameterValue(DynamicsCompressor::ParamReduction);
    m_reduction->setValue(reduction);
}

void DynamicsCompressorNode::reset(std::shared_ptr<AudioContext>)
{
    m_dynamicsCompressor->reset();
}

void DynamicsCompressorNode::initialize()
{
    if (isInitialized())
        return;

    AudioNode::initialize();    
    m_dynamicsCompressor = std::unique_ptr<DynamicsCompressor>(new DynamicsCompressor(sampleRate(), defaultNumberOfOutputChannels));
}

void DynamicsCompressorNode::uninitialize()
{
    if (!isInitialized())
        return;

    m_dynamicsCompressor.reset();
    AudioNode::uninitialize();
}

double DynamicsCompressorNode::tailTime() const
{
    return m_dynamicsCompressor->tailTime();
}

double DynamicsCompressorNode::latencyTime() const
{
    return m_dynamicsCompressor->latencyTime();
}

} // namespace WebCore
