// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/extended/RealtimeAnalyser.h"

namespace lab
{

struct AnalyserNode::Detail
{
    Detail() = default;
    ~Detail()
    {
        delete m_analyser;
    }
    RealtimeAnalyser* m_analyser = nullptr;

    std::shared_ptr<AudioSetting> _fftSize;
    std::shared_ptr<AudioSetting> _minDecibels;
    std::shared_ptr<AudioSetting> _maxDecibels;
    std::shared_ptr<AudioSetting> _smoothingTimeConstant;
};

void AnalyserNode::shared_construction(size_t fftSize)
{
    _detail = new Detail;

    _detail->m_analyser = new RealtimeAnalyser((uint32_t)fftSize);

    _detail->_fftSize = std::make_shared<AudioSetting>("fftSize", "FFTS", AudioSetting::Type::Integer);
    _detail->_minDecibels = std::make_shared<AudioSetting>("minDecibels", "MNDB", AudioSetting::Type::Float);
    _detail->_maxDecibels = std::make_shared<AudioSetting>("maxDecibels", "MXDB", AudioSetting::Type::Float);
    _detail->_smoothingTimeConstant = std::make_shared<AudioSetting>("smoothingTimeConstant", "STIM", AudioSetting::Type::Float);

    _detail->_fftSize->setUint32(static_cast<uint32_t>(fftSize));
    _detail->_fftSize->setValueChanged(
        [this]() {
            delete _detail->m_analyser;
            _detail->m_analyser = new RealtimeAnalyser(_detail->_fftSize->valueUint32());

            // restore other values
            _detail->m_analyser->setMinDecibels(_detail->_minDecibels->valueFloat());
            _detail->m_analyser->setMaxDecibels(_detail->_maxDecibels->valueFloat());
        });

    _detail->_minDecibels->setFloat(-100.f);
    _detail->_minDecibels->setValueChanged(
        [this]() {
            _detail->m_analyser->setMinDecibels(_detail->_minDecibels->valueFloat());
        });

    _detail->_maxDecibels->setFloat(-30.f);
    _detail->_maxDecibels->setValueChanged(
        [this]() {
            _detail->m_analyser->setMaxDecibels(_detail->_minDecibels->valueFloat());
        });

    _detail->_smoothingTimeConstant->setFloat(0.8f);
    _detail->_smoothingTimeConstant->setValueChanged(
        [this]() {
            _detail->m_analyser->setSmoothingTimeConstant(_detail->_smoothingTimeConstant->valueFloat());
        });

    m_settings.push_back(_detail->_fftSize);
    m_settings.push_back(_detail->_minDecibels);
    m_settings.push_back(_detail->_maxDecibels);
    m_settings.push_back(_detail->_smoothingTimeConstant);

    // N.B.: inputs and outputs added by AudioBasicInspectorNode... no need to create here.
    initialize();
}

AnalyserNode::AnalyserNode(size_t fftSize)
    : AudioBasicInspectorNode(1)
{
    shared_construction(fftSize);
}

AnalyserNode::AnalyserNode() 
: AudioBasicInspectorNode(1)
{
    shared_construction(1024u);
}

AnalyserNode::~AnalyserNode()
{
    uninitialize();
}

void AnalyserNode::setMinDecibels(double k) {
    _detail->_minDecibels->setFloat(static_cast<float>(k)); }
double AnalyserNode::minDecibels() const {
    return _detail->m_analyser->minDecibels(); }
void AnalyserNode::setMaxDecibels(double k) {
    _detail->_maxDecibels->setFloat(static_cast<float>(k)); }
double AnalyserNode::maxDecibels() const {
    return _detail->m_analyser->maxDecibels(); }
void AnalyserNode::setSmoothingTimeConstant(double k) {
    _detail->_smoothingTimeConstant->setFloat(static_cast<float>(k)); }
double AnalyserNode::smoothingTimeConstant() const {
    return _detail->m_analyser->smoothingTimeConstant(); }
void AnalyserNode::setFftSize(ContextRenderLock&, size_t sz) {
    _detail->_fftSize->setUint32(static_cast<uint32_t>(sz)); }
size_t AnalyserNode::fftSize() const { 
    return _detail->m_analyser->fftSize(); }
size_t AnalyserNode::frequencyBinCount() const { 
    return _detail->m_analyser->frequencyBinCount(); }
void AnalyserNode::getFloatFrequencyData(std::vector<float>& array) { 
    _detail->m_analyser->getFloatFrequencyData(array); }
void AnalyserNode::getByteFrequencyData(std::vector<uint8_t>& array) { 
    _detail->m_analyser->getByteFrequencyData(array); }
void AnalyserNode::getFloatTimeDomainData(std::vector<float>& array) { 
    _detail->m_analyser->getFloatTimeDomainData(array); } // LabSound
void AnalyserNode::getByteTimeDomainData(std::vector<uint8_t>& array) { 
    _detail->m_analyser->getByteTimeDomainData(array); }

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
    _detail->m_analyser->writeInput(r, inputBus, framesToProcess);

    // For in-place processing, our override of pullInputs() will just pass the audio data through unchanged if the channel count matches from input to output
    // (resulting in inputBus == outputBus). Otherwise, do an up-mix to stereo.
    if (inputBus != outputBus)
    {
        outputBus->copyFrom(*inputBus);
    }
}

void AnalyserNode::reset(ContextRenderLock&)
{
    _detail->m_analyser->reset();
}

}  // namespace lab
