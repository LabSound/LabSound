// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef MixNode_h
#define MixNode_h

#include "LabSound/core/AudioArray.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"

namespace lab
{
class AudioContext;

// Mixes two inputs based on the parameter value.
// 0.0 fully input0
// 1.0 fully input1
class MixNode : public AudioNode
{
public:
    MixNode(AudioContext& ac);
    virtual ~MixNode();

    static const char* static_name() { return "Mix"; }
    virtual const char* name() const override { return static_name(); }
    static AudioNodeDescriptor * desc();

    // AudioNode
    void process(ContextRenderLock&, int bufferSize) override;
    void reset(ContextRenderLock&) override;

    std::shared_ptr<AudioParam> mix() { return m_mix; }
protected:
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    std::shared_ptr<AudioParam> m_mix;
};

} // namespace lab

#endif // MixNode_h