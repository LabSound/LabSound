
// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/GainNode.h"
#include "LabSound/core/OscillatorNode.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/SupersawNode.h"
#include "LabSound/extended/Registry.h"
#include "LabSound/extended/VectorMath.h"

#include <cfloat>

using namespace lab;

namespace lab
{

//////////////////////////////////////////
// Private Supersaw Node Implementation //
//////////////////////////////////////////

static AudioParamDescriptor s_ssParams[] = {
    {"detune",    "DTUN",   1.0, 0,    120},
    {"frequency", "FREQ", 440.0, 0, 100000}, nullptr};
static AudioSettingDescriptor s_ssSettings[] = {{"sawCount", "SAWC", SettingType::Integer}, nullptr};

AudioNodeDescriptor * SupersawNode::desc()
{
    static AudioNodeDescriptor d {s_ssParams, s_ssSettings, 1};
    return &d;
}

class SupersawNode::SupersawNodeInternal
{
public:

    SupersawNodeInternal() = default;
    ~SupersawNodeInternal() = default;

    std::shared_ptr<AudioParam> detune;
    std::shared_ptr<AudioParam> frequency;
    std::shared_ptr<AudioSetting> sawCount;
    std::vector<std::vector<float>> phaseIncrements;
    std::vector<float> phases;
};

//////////////////////////
// Public Supersaw Node //
//////////////////////////

SupersawNode::SupersawNode(AudioContext & ac)
    : AudioScheduledSourceNode(ac, *desc())
{
    _internal.reset(new SupersawNodeInternal());
    _internal->sawCount = setting("sawCount");
    _internal->sawCount->setUint32(1);
    _internal->detune = param("detune");
    _internal->frequency = param("frequency");
    initialize();
}

SupersawNode::~SupersawNode()
{
    uninitialize();
}

void SupersawNode::process(ContextRenderLock & r, int bufferSize)
{
    AudioBus * outputBus = _self->output.get();
    if (!r.context() || !isInitialized() || !outputBus->numberOfChannels())
    {
        outputBus->zero();
        return;
    }
    
    const float sample_rate = r.context()->sampleRate();
    
    int quantumFrameOffset = _self->scheduler.renderOffset();
    int nonSilentFramesToProcess = _self->scheduler.renderLength();
    int voices = _internal->sawCount->valueUint32();

    if (!nonSilentFramesToProcess || !voices)
    {
        outputBus->zero();
        return;
    }
    
    // allocate storage for the sawtooths
    while (_internal->phaseIncrements.size() < voices) {
        _internal->phaseIncrements.push_back(std::vector<float>());
    }
    for (int i = 0; i < voices; ++i) {
        _internal->phaseIncrements[i].resize(bufferSize);
    }
    if (!_internal->phases.size()) {
        _internal->phases.resize(voices);
        for (int i = 0; i < voices; ++i)
            _internal->phases[i] = 0;
    }
    
    nonSilentFramesToProcess += quantumFrameOffset;
    
    float detuneRate[128];
    
    // calculate phase increments
    const float* frequencies = _internal->frequency->bus()->channel(0)->data();
    const float* detunes = _internal->detune->bus()->channel(0)->data();
    for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i) {
        detuneRate[i] = detunes[i];
    }
    
    // divide by number of voices, as the detunes will be spread evenly across the
    // sawtooth waves
    float k = 1.f / (1200.f * float(voices));
    VectorMath::vsmul(detuneRate + quantumFrameOffset, 1,
                      &k,
                      detuneRate + quantumFrameOffset, 1, nonSilentFramesToProcess);

    for (int v = 0; v < voices; ++v) {
        std::vector<float>& pi = _internal->phaseIncrements[v];
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i) {
            pi[i] = frequencies[i] * powf(2, detuneRate[i] * float(v));
        }
        // convert frequencies to phase increments
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
        {
            pi[i] = static_cast<float>(2.f * static_cast<float>(LAB_PI) * pi[i] / sample_rate);
        }
    }

    // calculate and write the wave
    float* destP = outputBus->channel(0)->mutableData();
    const float pi = static_cast<float>(LAB_PI);

    memset(destP, 0, sizeof(float) * bufferSize);
    float amp = 1.f / float(voices);
    for (int v = 0; v < voices; ++v) {
        std::vector<float>& pi = _internal->phaseIncrements[v];
        for (int i = quantumFrameOffset; i < nonSilentFramesToProcess; ++i)
        {
            destP[i] += amp - (amp / pi * _internal->phases[v]);
            _internal->phases[v] += pi[i];
            if (_internal->phases[v] > 2.f * pi)
                _internal->phases[v] -= 2.f * pi;
        }
    }
    
    outputBus->clearSilentFlag();
}

void SupersawNode::update(ContextRenderLock & r)
{
}

std::shared_ptr<AudioParam> SupersawNode::detune() const
{
    return _internal->detune;
}
std::shared_ptr<AudioParam> SupersawNode::frequency() const
{
    return _internal->frequency;
}
std::shared_ptr<AudioSetting> SupersawNode::sawCount() const
{
    return _internal->sawCount;
}

bool SupersawNode::propagatesSilence(ContextRenderLock & r) const
{
    return !isPlayingOrScheduled() || hasFinished();
}

}  // End namespace lab
