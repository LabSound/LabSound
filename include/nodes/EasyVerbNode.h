// Copyright (c) 2014 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "AudioBasicProcessorNode.h"
#include "AudioParam.h"
#include "STKNode.h"

namespace LabSound {

	class EasyVerbNode : public WebCore::AudioBasicProcessorNode {

	public:
        EasyVerbNode(std::shared_ptr<AudioContext>, float sampleRate);
		~EasyVerbNode();

		// In Seconds 
        std::shared_ptr<AudioParam> delayTime() const;

	private:

		class NodeInternal;
		NodeInternal* data;

	};

}
