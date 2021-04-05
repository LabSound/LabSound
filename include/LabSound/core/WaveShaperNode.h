// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef WaveShaperNode_h
#define WaveShaperNode_h

#include "LabSound/core/AudioBasicProcessorNode.h"

namespace lab
{
class WaveShaperProcessor;

class WaveShaperNode : public AudioNode
{
    WaveShaperProcessor * waveShaperProcessor();

public:
    WaveShaperNode(AudioContext & ac);
    virtual ~WaveShaperNode() = default;

    static const char* static_name() { return "WaveShaper"; }
    virtual const char* name() const override { return static_name(); }

    // setCurve will take ownership of curve
    void setCurve(std::vector<float> && curve);
    std::vector<float> & curve();

    // AudioNode
    virtual void process(ContextRenderLock &, int bufferSize) override;
    virtual void reset(ContextRenderLock &) override;
    virtual void initialize() override;
    virtual void uninitialize() override;

    // Returns the number of channels for both the input and the output.
    int numberOfChannels();

protected:
    virtual double tailTime(ContextRenderLock & r) const override;
    virtual double latencyTime(ContextRenderLock & r) const override;

    std::unique_ptr<AudioProcessor> m_processor;
};

}  // namespace lab

#endif
