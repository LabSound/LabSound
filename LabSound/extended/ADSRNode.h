// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef LabSound_ADSRNode_h_impl
#define LabSound_ADSRNode_h_impl

#include "AudioContext.h"
#include "AudioBus.h"
#include "AudioParam.h"

#include "AudioScheduledSourceNode.h"
#include "AudioBasicProcessorNode.h"
#include "AudioBasicInspectorNode.h"
#include "GainNode.h"

namespace LabSound
{

    class ADSRNode : public WebCore::AudioBasicProcessorNode
    {
        class ADSRNodeInternal;
        ADSRNodeInternal * internalNode;
        
    public:
        
        ADSRNode(float sampleRate);
        virtual ~ADSRNode();
        
        // If noteOn is called before noteOff has finished, a pop can occur. Polling
        // finished and avoiding noteOn while finished is true can avoid the popping.
		void noteOn(double when);
		void noteOff(ContextRenderLock&, double when);
        
        bool finished(ContextRenderLock&); // if a noteOff has been issued, finished will be true after the release period

		void set(float aT, float aL, float d, float s, float r);

        std::shared_ptr<WebCore::AudioParam> attackTime() const; // Duration in ms
        std::shared_ptr<WebCore::AudioParam> attackLevel() const; // Duration in ms
        std::shared_ptr<WebCore::AudioParam> decayTime() const; // Duration in ms
        std::shared_ptr<WebCore::AudioParam> sustainLevel() const; // Level 0-10
        std::shared_ptr<WebCore::AudioParam> releaseTime() const; // Duration in ms
    };
    
}

#endif
