// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef FUNCTION_NODE_H
#define FUNCTION_NODE_H

#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"

namespace lab
{
    
    class FunctionNode : public AudioScheduledSourceNode
    {
        
    public:
        
        FunctionNode(float sampleRate, int channels);
        virtual ~FunctionNode();
        
        void setFunction(std::function<void(ContextRenderLock&, FunctionNode*, int, float*, size_t)> fn)
        {
            _function = fn;
        }
        
        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;
        
        double now() const { return _now; }
        
    private:
        
        virtual bool propagatesSilence(double now) const override;
        
        std::function<void(ContextRenderLock& r, FunctionNode *self,
                           int channel, float* values, size_t framesToProcess)> _function;
        double _now = 0.0;
    };
    
} // end namespace lab

#endif
