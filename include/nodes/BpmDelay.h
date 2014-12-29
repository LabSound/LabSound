// Copyright (c) 2014 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "AudioBasicProcessorNode.h"
#include "AudioParam.h"
#include "STKNode.h"

namespace LabSound {

	class BpmDelay : public WebCore::AudioBasicProcessorNode {

	public:

        BpmDelay(std::shared_ptr<AudioContext>, float sampleRate);
		~BpmDelay();

		// In Seconds 
		AudioParam* delayTime() const; 

	private:


		class NodeInternal;
		NodeInternal* data;

	};

}
