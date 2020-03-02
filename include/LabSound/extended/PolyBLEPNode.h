// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020+, The LabSound Authors. All rights reserved.

#ifndef lab_poly_blep_node_h
#define lab_poly_blep_node_h

#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/Macros.h"
#include "LabSound/core/WaveTable.h"

namespace lab
{

class AudioBus;
class AudioContext;
class AudioSetting;

enum class PolyBLEPType
{
    TRIANGLE,
    SQUARE,
    RECTANGLE,
    SAWTOOTH,
    RAMP,
    MODIFIED_TRIANGLE,
    MODIFIED_SQUARE,
    HALF_WAVE_RECTIFIED_SINE,
    FULL_WAVE_RECTIFIED_SINE,
    TRIANGULAR_PULSE,
    TRAPEZOID_FIXED,
    TRAPEZOID_VARIABLE,
    _PolyBLEPCount
};

class PolyBlepImpl;

class PolyBLEPNode : public AudioScheduledSourceNode
{
    bool m_firstRender {true};
    double phase = 0.0;
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
    virtual bool propagatesSilence(ContextRenderLock & r) const override;
    std::shared_ptr<AudioSetting> m_type;
    std::unique_ptr<PolyBlepImpl> polyblep;

public:
    PolyBLEPNode();
    virtual ~PolyBLEPNode();

    virtual void process(ContextRenderLock &, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock &) override { }

    PolyBLEPType type() const;
    void setType(PolyBLEPType type);

    std::shared_ptr<AudioParam> amplitude() { return m_amplitude; }
    std::shared_ptr<AudioParam> frequency() { return m_frequency; }

    std::shared_ptr<AudioParam> m_amplitude; // default 1.0
    std::shared_ptr<AudioParam> m_frequency; // hz

    void processPolyBLEP(ContextRenderLock & r, const size_t frames);

    AudioFloatArray m_amplitudeValues;
};

}  // namespace lab

#endif  // lab_poly_blep_node_h
