
#include "LabSound/extended/BPMDelay.h"

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"
#include "LabSound/extended/AudioContextLock.h"
#include "internal/AudioBus.h"

#include <iostream>
#include <vector>

using namespace WebCore;

namespace LabSound 
{

    BPMDelay::BPMDelay(float sampleRate) : WebCore::DelayNode(sampleRate, 64)
    {
        setNodeType((AudioNode::NodeType) LabSound::NodeType::NodeTypeBPMDelay);

        addInput(std::unique_ptr<AudioNodeInput>(new WebCore::AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new WebCore::AudioNodeOutput(this, 2)));

		times = 
		{
			1.f / 8.f,
			(1.f / 4.f) * 2.f / 3.f,
			(1.f / 8.f) * 3.f / 2.f,
			1.f / 4.f,
			(1.f / 2.f) * 2.f / 3.f, 
			(1.f / 4.f) * 3.f / 2.f,
			1.f / 2.f, 
			1.f * 2.f / 3.f,
			(1.f / 2.f) * 3.f / 2.f, 
			1.0f, 
			2.f * 2.f / 3.f, 
			1.f * 3.f / 2.f, 
			2.f, 
			3.f
		};

		SetDelayIndex(TempoSync::TS_8);

        initialize();
    }
    
    BPMDelay::~BPMDelay()
    {
        uninitialize();
    }

} // end namespace LabSound

