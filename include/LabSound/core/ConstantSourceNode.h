// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020+, The LabSound Authors. All rights reserved.

#ifndef lab_constant_node_h
#define lab_constant_node_h

#include "LabSound/core/AudioArray.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/Macros.h"

namespace lab
{

class AudioBus;
class AudioContext;
class AudioSetting;

class ConstantSourceNode : public AudioScheduledSourceNode
{
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
    virtual bool propagatesSilence(ContextRenderLock & r) const override;

public:
    ConstantSourceNode(AudioContext & ac);
    virtual ~ConstantSourceNode();

    static const char * static_name() { return "ConstantSourceNode"; }
    virtual const char * name() const override { return static_name(); }
    static AudioNodeDescriptor * desc();
    virtual void process(ContextRenderLock &, int bufferSize) override;
    virtual void reset(ContextRenderLock &) override { }
    std::shared_ptr<AudioParam> offset() { return m_offset; }

    protected:

    void process_internal(ContextRenderLock & r, int bufferSize, int offset, int count);

    std::shared_ptr<AudioParam> m_offset;  // default 1.0

    AudioFloatArray m_sampleAccurateOffsetValues;
};

}  // namespace lab

#endif  // lab_constant_source_node_h
