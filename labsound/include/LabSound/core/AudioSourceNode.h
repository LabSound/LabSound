// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioSourceNode_h
#define AudioSourceNode_h

#include "LabSound/core/AudioNode.h"

namespace lab {

class AudioSourceNode : public AudioNode 
{

public:

    AudioSourceNode() : AudioNode() { }
    virtual ~AudioSourceNode() {}
    
protected:

    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
};

} // namespace lab

#endif // AudioSourceNode_h
