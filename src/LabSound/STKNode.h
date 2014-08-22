//
//  STKNode.h
//
// Copyright (c) 2014 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#ifndef __LabSound__STKNode__
#define __LabSound__STKNode__

#include "../Modules/webaudio/AudioContext.h"
#include "../Modules/webaudio/AudioNode.h"
#include "../Modules/webaudio/AudioParam.h"
#include "ADSRNode.h"
#include "STKIncludes.h"
#include <direct.h>

namespace LabSound {

    using namespace WebCore;

	class STKNode : public AudioScheduledSourceNode {

    public:

		static PassRefPtr<STKNode> create(AudioContext* context, float sampleRate) {

			// Oops, windows for now...
			char cwd[MAX_PATH];
			_getcwd(cwd, MAX_PATH);

			std::string resourcePath = std::string(cwd) + std::string("\\stkresources\\");
			std::cout << "STK Resource Path: " << resourcePath << std::endl;
			stk::Stk::setRawwavePath(resourcePath);

			return adoptRef(new STKNode(context, sampleRate));

		}

		AudioParam* attack()  const;
		AudioParam* decay()   const;
		AudioParam* sustain() const;
		AudioParam* release() const;

		void noteOn(float frequency);
		void noteOff();

        void update(); 

    private:
        
		STKNode(WebCore::AudioContext*, float sampleRate);

        // Satisfy the AudioNode interface
        virtual void process(size_t);
        virtual void reset() { /*m_currentSampleFrame = 0;*/ }

		// virtual double tailTime() const OVERRIDE { return 0; }
		// virtual double latencyTime() const OVERRIDE { return 0; }

        virtual bool propagatesSilence() const OVERRIDE;

		RefPtr<ADSRNode> gainNode; 

		stk::BeeThree synth;

    };
}

#endif
