// License: BSD 2 Clause
// Copyright (C) 2014, The Chromium Authors. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef StereoPannerNode_h
#define StereoPannerNode_h

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioArray.h"

#include <memory>
#include <string>

namespace lab
{

class Spatializer;
    
// StereoPannerNode is an AudioNode with one input and one output. It is
// specifically designed for equal-power stereo panning.
class StereoPannerNode : public AudioNode
{
public:
    
    StereoPannerNode(float sampleRate);
    
    virtual ~StereoPannerNode();
    
    std::shared_ptr<AudioParam> pan() { return m_pan; }
    
private:
    
    // AudioNode
    virtual void process(ContextRenderLock &, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock &) override;
    
    virtual void initialize() override;
    virtual void uninitialize() override;
    
    
    virtual double tailTime() const override { return 0; }
    virtual double latencyTime() const override { return 0; }
    
    std::shared_ptr<AudioParam> m_pan;
    
    std::unique_ptr<Spatializer> m_stereoPanner;
    std::unique_ptr<AudioFloatArray> m_sampleAccuratePanValues;
    
};
    
} // namespace lab

#endif // StereoPannerNode_h
