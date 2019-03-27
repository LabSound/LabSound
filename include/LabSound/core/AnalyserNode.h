// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AnalyserNode_h
#define AnalyserNode_h

#include "LabSound/core/AudioBasicInspectorNode.h"

//@fixme this node is actually extended
#include "LabSound/extended/RealtimeAnalyser.h"

namespace lab {

class AudioSetting;

// params:
// settings: fftSize, minDecibels, maxDecibels, smoothingTimeConstant
//
class AnalyserNode : public AudioBasicInspectorNode 
{
public:

    AnalyserNode(); // defaults to 1024
    AnalyserNode(size_t fftSize);
    virtual ~AnalyserNode();
    
    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;

    void setFftSize(ContextRenderLock&, size_t fftSize);
    size_t fftSize() const { return m_analyser->fftSize(); }

    size_t frequencyBinCount() const { return m_analyser->frequencyBinCount(); }

    void setMinDecibels(double k);
    double minDecibels() const;

    void setMaxDecibels(double k);
    double maxDecibels() const;

    void setSmoothingTimeConstant(double k);
    double smoothingTimeConstant() const;

    // ffi: user facing functions
    void getFloatFrequencyData(std::vector<float>& array) { m_analyser->getFloatFrequencyData(array); }
    void getByteFrequencyData(std::vector<uint8_t>& array) { m_analyser->getByteFrequencyData(array); }
    void getFloatTimeDomainData(std::vector<float>& array) { m_analyser->getFloatTimeDomainData(array); } // LabSound
    void getByteTimeDomainData(std::vector<uint8_t>& array) { m_analyser->getByteTimeDomainData(array); }

private:
    void shared_construction(size_t fftSize);

    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    RealtimeAnalyser* m_analyser;

    std::shared_ptr<AudioSetting> _fftSize;
    std::shared_ptr<AudioSetting> _minDecibels;
    std::shared_ptr<AudioSetting> _maxDecibels;
    std::shared_ptr<AudioSetting> _smoothingTimeConstant;
};

} // namespace lab

#endif // AnalyserNode_h
