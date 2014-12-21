// Copyright (c) 2014 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "AudioBasicProcessorNode.h"
#include "AudioParam.h"
#include "STKNode.h"

namespace LabSound {

	class EasyVerbNode : public WebCore::AudioBasicProcessorNode {

	public:

		static WTF::PassRefPtr<EasyVerbNode> create(std::shared_ptr<AudioContext> context, float sampleRate) {
			return adoptRef(new EasyVerbNode(context, sampleRate));
		}

		~EasyVerbNode(); 

		// In Seconds 
		AudioParam* delayTime() const; 

	private:

		EasyVerbNode(std::shared_ptr<AudioContext>, float sampleRate);

		class NodeInternal;
		NodeInternal* data;

	};

}
