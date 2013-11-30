
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

        // Could be better. Power is computed on the most recent frame. If the framesize is greater
        // than the windowSize, then power is returned for the windowed end-of-frame. If framesize
        // is less than windowSize, power is computed only on framesize. This could be a problem
        // if the framesize is very large compared to the sample rate, for example, a 4k framesize
        // on a 44khz samplerate is going to give you usable power measurements 11 times a second.
        // If better resolution is required, it's probably better to use a RecorderNode, and perform
        // analysis on the full data stream pulled from it.
        //
        // The intent of the power monitor node is to provide levels that can be used for a VU meter
        // or a ducking algorithm.
        //
        void windowSize(size_t ws) { _windowSize = ws; }
        size_t windowSize() const { return _windowSize; }
        
    private:
        virtual double tailTime() const OVERRIDE { return 0; }      // required for BasicInspector
        virtual double latencyTime() const OVERRIDE { return 0; }   // required for BasicInspector

        float _db;
        size_t _windowSize;
        PowerMonitorNode(WebCore::AudioContext*, float sampleRate);
    };

} // LabSound
