// Copyright (c) 2014 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "../Modules/webaudio/AudioBasicProcessorNode.h"
#include "../Modules/webaudio/AudioParam.h"
#include "STKNode.h"

namespace LabSound {

	class BpmDelay : public WebCore::AudioBasicProcessorNode {

	public:

		static WTF::PassRefPtr<BpmDelay> create(WebCore::AudioContext* context, float sampleRate) {
			return adoptRef(new BpmDelay(context, sampleRate));
		}

		~BpmDelay(); 

		// In Seconds 
		AudioParam* delayTime() const; 

	private:

		BpmDelay(WebCore::AudioContext*, float sampleRate);

		class NodeInternal;
		NodeInternal* data;

	};

}
