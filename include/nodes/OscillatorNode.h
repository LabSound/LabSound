/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
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

#ifndef OscillatorNode_h
#define OscillatorNode_h

#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"

#include "AudioBus.h"

namespace WebCore {

class AudioContext;
class WaveTable;
    
class OscillatorNode : public AudioScheduledSourceNode
{
    
public:
    enum
    {
        SINE = 0,
        SQUARE = 1,
        SAWTOOTH = 2,
        TRIANGLE = 3,
        CUSTOM = 4
    };

    OscillatorNode(ContextRenderLock& r, float sampleRate);
    virtual ~OscillatorNode();
    
    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(std::shared_ptr<AudioContext>) override;

    unsigned short type() const { return m_type; }
    void setType(ContextRenderLock& r, unsigned short, ExceptionCode&);

    std::shared_ptr<AudioParam> frequency() { return m_frequency; }
    std::shared_ptr<AudioParam> detune() { return m_detune; }

    void setWaveTable(ContextRenderLock& r, std::shared_ptr<WaveTable>);

private:

    // Returns true if there are sample-accurate timeline parameter changes.
    bool calculateSampleAccuratePhaseIncrements(ContextRenderLock&, size_t framesToProcess);

    virtual bool propagatesSilence(double now) const override;

    // One of the waveform types defined in the enum.
    unsigned short m_type;
    
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

} // namespace WebCore

#endif // OscillatorNode_h
