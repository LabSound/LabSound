// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef OscillatorNode_h
#define OscillatorNode_h

#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/Macros.h"
#include "LabSound/core/WaveTable.h"

namespace lab
{

class AudioBus;
class AudioContext;
class AudioSetting;

// params: frequency, detune
// settings: type
//
class OscillatorNode : public AudioScheduledSourceNode
{
    bool m_firstRender {true};
    double phase = 0.0;
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
    virtual bool propagatesSilence(ContextRenderLock & r) const override;
    std::shared_ptr<AudioSetting> m_type;

public:
    OscillatorNode();
    virtual ~OscillatorNode();

    virtual void process(ContextRenderLock &, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock &) override { }

    OscillatorType type() const;
    void setType(OscillatorType type);

    std::shared_ptr<AudioParam> amplitude() { return m_amplitude; }
    std::shared_ptr<AudioParam> frequency() { return m_frequency; }
    std::shared_ptr<AudioParam> detune() { return m_detune; }
    std::shared_ptr<AudioParam> bias() { return m_bias; }

    std::shared_ptr<AudioParam> m_amplitude; // default 1.0
    std::shared_ptr<AudioParam> m_frequency; // hz
    std::shared_ptr<AudioParam> m_bias;      // default 0.0
    std::shared_ptr<AudioParam> m_detune;    // Detune value in Cents.

    void process_oscillator(ContextRenderLock & r, const size_t frames);

    AudioFloatArray m_phaseIncrements;
    AudioFloatArray m_biasValues;
    AudioFloatArray m_detuneValues;
    AudioFloatArray m_amplitudeValues;
};

}  // namespace lab

#endif  // OscillatorNode_h
