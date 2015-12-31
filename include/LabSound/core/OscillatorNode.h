// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef OscillatorNode_h
#define OscillatorNode_h

#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/WaveTable.h"
#include "LabSound/core/Synthesis.h"

namespace lab {

class AudioContext;
class AudioBus;

class OscillatorNode : public AudioScheduledSourceNode
{
    
public:
    
    OscillatorNode(ContextRenderLock& r, float sampleRate);
    virtual ~OscillatorNode();
    
    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;

    OscillatorType type() const { return m_type; }
    void setType(ContextRenderLock& r, OscillatorType);

    std::shared_ptr<AudioParam> frequency() { return m_frequency; }
    std::shared_ptr<AudioParam> detune() { return m_detune; }

    void setWaveTable(ContextRenderLock& r, std::shared_ptr<WaveTable>);

private:

    void setType(bool isConstructor, OscillatorType);
    void setWaveTable(bool isConstructor, std::shared_ptr<WaveTable>);

    // Returns true if there are sample-accurate timeline parameter changes.
    bool calculateSampleAccuratePhaseIncrements(ContextRenderLock&, size_t framesToProcess);

    virtual bool propagatesSilence(double now) const override;

    // One of the waveform types defined in the enum.
    OscillatorType m_type;
    
    // Frequency value in Hertz.
    std::shared_ptr<AudioParam> m_frequency;

    // Detune value (deviating from the frequency) in Cents.
    std::shared_ptr<AudioParam> m_detune;

    bool m_firstRender;

    // m_virtualReadIndex is a sample-frame index into our buffer representing the current playback position.
    // Since it's floating-point, it has sub-sample accuracy.
    double m_virtualReadIndex;

    // Stores sample-accurate values calculated according to frequency and detune.
    AudioFloatArray m_phaseIncrements;
    AudioFloatArray m_detuneValues;
    
    std::shared_ptr<WaveTable> m_waveTable;

    // Cache the wave tables for different waveform types, except CUSTOM.
    static std::shared_ptr<WaveTable> s_waveTableSine;
    static std::shared_ptr<WaveTable> s_waveTableSquare;
    static std::shared_ptr<WaveTable> s_waveTableSawtooth;
    static std::shared_ptr<WaveTable> s_waveTableTriangle;
};

} // namespace lab

#endif // OscillatorNode_h
