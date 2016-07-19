// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AnalyserNode_h
#define AnalyserNode_h

#include "LabSound/core/AudioBasicInspectorNode.h"

//@fixme this node is actually extended
#include "LabSound/extended/RealtimeAnalyser.h"

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
