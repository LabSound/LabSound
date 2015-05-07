// Copyright (c) 2003-2015 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef FunctionNode_h
#define FunctionNode_h

#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"

namespace LabSound
{
    
    class FunctionNode : public WebCore::AudioScheduledSourceNode
    {
        
    public:
        
        FunctionNode(float sampleRate, int channels); // empty constructor == bad
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
        double _now;
    };
    
} // end namespace LabSound

#endif
