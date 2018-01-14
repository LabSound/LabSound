// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ChannelSplitterNode_h
#define ChannelSplitterNode_h

#include "LabSound/core/AudioNode.h"

namespace lab
{

class AudioContext;
    
class ChannelSplitterNode : public AudioNode
{
        
public:

    ChannelSplitterNode(unsigned numberOfOutputs);
    virtual ~ChannelSplitterNode() {}
    
    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;

private:

    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

};

} // namespace lab

#endif // ChannelSplitterNode_h
