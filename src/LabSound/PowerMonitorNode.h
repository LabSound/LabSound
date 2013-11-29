
// Copyright (c) 2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT
#pragma once


#include "AudioBasicInspectorNode.h"
#include <wtf/Forward.h>
#include <vector>
#include <mutex>

namespace LabSound {
    
    class PowerMonitorNode : public WebCore::AudioBasicInspectorNode {
    public:
        static WTF::PassRefPtr<PowerMonitorNode> create(WebCore::AudioContext* context, float sampleRate)
        {
            return adoptRef(new PowerMonitorNode(context, sampleRate));
        }
        
        virtual ~PowerMonitorNode();
        
        // AudioNode
        virtual void process(size_t framesToProcess);
        virtual void reset();
        // ..AudioNode
        
        float db() const { return _db; }
        
        
    private:
        virtual double tailTime() const OVERRIDE { return 0; }      // required for BasicInspector
        virtual double latencyTime() const OVERRIDE { return 0; }   // required for BasicInspector

        float _db;
        PowerMonitorNode(WebCore::AudioContext*, float sampleRate);
    };

} // LabSound
