// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"

namespace lab
{

void AnalyserNode::shared_construction(size_t fftSize)
{
    _fftSize = std::make_shared<AudioSetting>("fftSize");
    _minDecibels = std::make_shared<AudioSetting>("minDecibels");
    _maxDecibels = std::make_shared<AudioSetting>("maxDecibels");
    _smoothingTimeConstant = std::make_shared<AudioSetting>("smoothingTimeConstant");

    _fftSize->setUint32(static_cast<uint32_t>(fftSize));
    _fftSize->setValueChanged(
        [this]() {
            delete m_analyser;
            m_analyser = new RealtimeAnalyser(_fftSize->valueUint32());

            // restore other values
            m_analyser->setMinDecibels(_minDecibels->valueFloat());
            m_analyser->setMaxDecibels(_maxDecibels->valueFloat());
        });

    _minDecibels->setFloat(-100.f);
    _minDecibels->setValueChanged(
        [this]() {
            m_analyser->setMinDecibels(_minDecibels->valueFloat());
        });

    _maxDecibels->setFloat(-30.f);
    _maxDecibels->setValueChanged(
        [this]() {
            m_analyser->setMaxDecibels(_minDecibels->valueFloat());
        });

    _smoothingTimeConstant->setFloat(0.8f);
    _smoothingTimeConstant->setValueChanged(
        [this]() {
            m_analyser->setSmoothingTimeConstant(_smoothingTimeConstant->valueFloat());
        });

    m_settings.push_back(_fftSize);
    m_settings.push_back(_minDecibels);
    m_settings.push_back(_maxDecibels);
    m_settings.push_back(_smoothingTimeConstant);

    // N.B.: inputs and outputs added by AudioBasicInspectorNode... no need to create here.
    initialize();
}

AnalyserNode::AnalyserNode(size_t fftSize)
    : AudioBasicInspectorNode((uint32_t) 2)
    , m_analyser(new RealtimeAnalyser((uint32_t) fftSize))
{
    shared_construction(fftSize);
}

AnalyserNode::AnalyserNode() 
: AudioBasicInspectorNode((uint32_t) 2)
, m_analyser(new RealtimeAnalyser(1024u))
{
    shared_construction(1024u);
}

AnalyserNode::~AnalyserNode()
{
    uninitialize();
}

void AnalyserNode::setMinDecibels(double k) {
    _minDecibels->setFloat(static_cast<float>(k)); }
double AnalyserNode::minDecibels() const {
    return m_analyser->minDecibels(); }
void AnalyserNode::setMaxDecibels(double k) {
    _maxDecibels->setFloat(static_cast<float>(k)); }
double AnalyserNode::maxDecibels() const {
    return m_analyser->maxDecibels(); }
void AnalyserNode::setSmoothingTimeConstant(double k) {
    _smoothingTimeConstant->setFloat(static_cast<float>(k)); }
double AnalyserNode::smoothingTimeConstant() const {
    return m_analyser->smoothingTimeConstant(); }
void AnalyserNode::setFftSize(ContextRenderLock&, size_t sz) {
    _fftSize->setUint32(static_cast<uint32_t>(sz)); }

void AnalyserNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    AudioBus * outputBus = output(0)->bus(r);

    if (!isInitialized() || !input(0)->isConnected())
    {
        outputBus->zero();
        return;
    }

    AudioBus * inputBus = input(0)->bus(r);

    // Give the analyser the audio which is passing through this AudioNode.
    m_analyser->writeInput(r, inputBus, framesToProcess);

    // For in-place processing, our override of pullInputs() will just pass the audio data through unchanged if the channel count matches from input to output
    // (resulting in inputBus == outputBus). Otherwise, do an up-mix to stereo.
    if (inputBus != outputBus)
    {
        outputBus->copyFrom(*inputBus);
    }
}

void AnalyserNode::reset(ContextRenderLock&)
{
    m_analyser->reset();
}

}  // namespace lab
