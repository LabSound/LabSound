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

    virtual void process(ContextRenderLock &, int bufferSize, int offset, int count) override;
    virtual void reset(ContextRenderLock &) override { }

    OscillatorType type() const;
    void setType(OscillatorType type);

    std::shared_ptr<AudioParam> amplitude() { return m_amplitude; }
    std::shared_ptr<AudioParam> frequency() { return m_frequency; }
    std::shared_ptr<AudioParam> detune() { return m_detune; }
    std::shared_ptr<AudioParam> bias() { return m_bias; }

protected:
    void _setType(OscillatorType type);

        float m_sampleRate;

    double _lab_phase = 0;  // new sine oscillator

    void process_oscillator(ContextRenderLock & r, int bufferSize, int offset, int count);

    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    // Returns true if there are sample-accurate timeline parameter changes.
    bool calculateSampleAccuratePhaseIncrements(ContextRenderLock &, size_t framesToProcess);

    // Returns true if there are sample-accurate timeline parameter changes.
    bool calculateSampleAccuratePhaseIncrements(ContextRenderLock &, size_t framesToProcess);

    void process_oscillator(ContextRenderLock & r, int frames);

    // One of the waveform types defined in the enum.
    std::shared_ptr<AudioSetting> m_type;

    bool m_firstRender;

    bool m_firstRender;

    // m_virtualReadIndex is a sample-frame index into the buffer
    double m_virtualReadIndex;

        // Cache the wave tables for different waveform types, except CUSTOM.
        static std::shared_ptr<WaveTable> s_waveTableSine;
        static std::shared_ptr<WaveTable> s_waveTableSquare;
        static std::shared_ptr<WaveTable> s_waveTableSawtooth;
        static std::shared_ptr<WaveTable> s_waveTableTriangle;
    };

}  // namespace lab

#endif  // OscillatorNode_h

}  // namespace lab

#endif  // OscillatorNode_h
