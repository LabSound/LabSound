/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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

#ifndef AnalyserNode_h
#define AnalyserNode_h

#include "LabSound/core/AudioBasicInspectorNode.h"

//@fixme this node is actually extended
#include "LabSound/Extended/RealtimeAnalyser.h"

namespace lab {

class AnalyserNode : public AudioBasicInspectorNode {
public:
    AnalyserNode(float sampleRate, size_t fftSize);
    virtual ~AnalyserNode();
    
    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;

    size_t fftSize() const { return m_analyser.fftSize(); }

    size_t frequencyBinCount() const { return m_analyser.frequencyBinCount(); }

    void setMinDecibels(double k) { m_analyser.setMinDecibels(k); }
    double minDecibels() const { return m_analyser.minDecibels(); }

    void setMaxDecibels(double k) { m_analyser.setMaxDecibels(k); }
    double maxDecibels() const { return m_analyser.maxDecibels(); }

    void setSmoothingTimeConstant(double k) { m_analyser.setSmoothingTimeConstant(k); }
    double smoothingTimeConstant() const { return m_analyser.smoothingTimeConstant(); }

    void getFloatFrequencyData(std::vector<float>& array) { m_analyser.getFloatFrequencyData(array); }
    void getByteFrequencyData(std::vector<uint8_t>& array) { m_analyser.getByteFrequencyData(array); }
    void getFloatTimeDomainData(std::vector<float>& array) { m_analyser.getFloatTimeDomainData(array); } // LabSound
    void getByteTimeDomainData(std::vector<uint8_t>& array) { m_analyser.getByteTimeDomainData(array); }

private:
    virtual double tailTime() const override { return 0; }
    virtual double latencyTime() const override { return 0; }

    RealtimeAnalyser m_analyser;
};

} // namespace lab

#endif // AnalyserNode_h
