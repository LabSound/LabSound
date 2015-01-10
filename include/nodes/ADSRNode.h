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

namespace LabSound {
    using namespace WebCore;

    class ADSRNode : public WebCore::AudioBasicProcessorNode
    {
    public:
        ADSRNode(std::shared_ptr<AudioContext>, float sampleRate);
        virtual ~ADSRNode();
        
        // If noteOn is called before noteOff has finished, a pop can occur. Polling
        // finished and avoiding noteOn while finished is true can avoid the popping.
        //
		void noteOn();
		void noteOff();
        bool finished(); // if a noteOff has been issued, finished will be true after the release period

		void set(float aT, float aL, float d, float s, float r);

        std::shared_ptr<AudioParam> attackTime() const;
		std::shared_ptr<AudioParam> attackLevel() const;
		std::shared_ptr<AudioParam> decayTime() const;
		std::shared_ptr<AudioParam> sustainLevel() const;
		std::shared_ptr<AudioParam> releaseTime() const;

    private:
        class AdsrNodeInternal;
        AdsrNodeInternal* data;
    };
    
}

#endif
