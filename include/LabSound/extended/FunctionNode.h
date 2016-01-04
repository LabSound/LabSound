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
        
        // @tofix - should this be protected with a mutex?
        void setFunction(std::function<void(ContextRenderLock & r, FunctionNode * me, int channel, float * buffer, size_t frames)> fn)
        {
            _function = fn;
        }
        
        virtual void process(ContextRenderLock & r, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock & r) override;
        
        double now() const { return _now; }
        
    private:
        
        virtual bool propagatesSilence(double now) const override;
        
        std::function<void(ContextRenderLock & r, FunctionNode * me, int channel, float * values, size_t framesToProcess)> _function;
        
        double _now = 0.0;
    };
    
} // end namespace lab

#endif
