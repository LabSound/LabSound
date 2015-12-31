// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ChannelMergerNode_h
#define ChannelMergerNode_h

#include "LabSound/core/AudioNode.h"

namespace lab {

class AudioContext;
    
class ChannelMergerNode : public AudioNode {
public:
    ChannelMergerNode(float sampleRate, unsigned numberOfInputs);
    virtual ~ChannelMergerNode() {}

    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;

    // Called in the audio thread (pre-rendering task) when the number of channels for an input may have changed.
    virtual void checkNumberOfChannelsForInput(ContextRenderLock&, AudioNodeInput*) override;

private:
    virtual double tailTime() const override { return 0; }
    virtual double latencyTime() const override { return 0; }
    
    uint32_t m_desiredNumberOfOutputChannels = 1; // default
};

} // namespace lab

#endif // ChannelMergerNode_h
