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
    public:
        OscillatorNode(const float sampleRate);
        virtual ~OscillatorNode();

        virtual void process(ContextRenderLock &, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock &) override;

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

        std::shared_ptr<AudioParam> m_amplitude;  // default 1
        std::shared_ptr<AudioParam> m_frequency;  // hz
        std::shared_ptr<AudioParam> m_bias;       // default 0

        // Detune value (deviating from the frequency) in Cents.
        std::shared_ptr<AudioParam> m_detune;

        virtual double tailTime(ContextRenderLock & r) const override { return 0; }
        virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

        virtual bool propagatesSilence(ContextRenderLock & r) const override;

        void process_oscillator(ContextRenderLock & r, int frames);

        // One of the waveform types defined in the enum.
        std::shared_ptr<AudioSetting> m_type;

        bool m_firstRender {true};

        AudioFloatArray m_phaseIncrements;
        AudioFloatArray m_biasValues;
        AudioFloatArray m_detuneValues;
        AudioFloatArray m_amplitudeValues;
    };

}  // namespace lab

#endif  // OscillatorNode_h
