// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef LabSound_ADSRNode_h
#define LabSound_ADSRNode_h

#include "AudioContext.h"
#include "AudioBus.h"
#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"
#include "AudioBasicInspectorNode.h"
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include "GainNode.h"

namespace LabSound {
    using namespace WebCore;

    class ADSRNode : public WebCore::GainNode {

    public:

		static PassRefPtr<ADSRNode> create(AudioContext* context, float sampleRate)
		{
			return adoptRef(new ADSRNode(context, sampleRate));      
		}

		virtual ~ADSRNode(); 

		AudioParam* attack()  const { return m_attack.get(); }
		AudioParam* decay()   const { return m_decay.get(); }
		AudioParam* sustain() const { return m_sustain.get(); }
		AudioParam* release() const { return m_release.get(); }

        // If noteOn is called before noteOff has finished, a pop can occur. Polling
        // finished and avoiding noteOn while finished is true can avoid the popping.
        //
		void noteOn();
		void noteOff();
        bool finished(); // if a noteOff has been issued, finished will be true after the release period

		void set(float a, float d, float s, float r);

    private:

		ADSRNode(AudioContext*, float sampleRate);

		RefPtr<AudioParam> m_attack;
		RefPtr<AudioParam> m_decay;
		RefPtr<AudioParam> m_sustain;
		RefPtr<AudioParam> m_release;

        double m_noteOffTime;
    };
    
}

#endif
