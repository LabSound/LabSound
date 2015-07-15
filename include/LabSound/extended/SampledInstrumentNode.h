
// Copyright (c) 2014 Dimitri Diakopolous, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioBufferSourceNode.h"

#include "LabSound/extended/ADSRNode.h"
#include "LabSound/extended/SoundBuffer.h"
#include "LabSound/extended/AudioContextLock.h"

#include <iostream> 
#include <array>
#include <string>
#include <algorithm>

namespace LabSound 
{
	struct SamplerSound;

	// This class is a little but subversive of the typical LabSound node pattern. 
	// Instead of inheriting from a node and injecting samples into an audio buffer,
	// it internally creates ad-hoc AudioBufferSourceNode(s) and keeps them around
	// in the graph so long as our gain node has been connected. 
	class SampledInstrumentNode 
	{
		std::vector<std::shared_ptr<SamplerSound>> samples;
		std::shared_ptr<WebCore::GainNode> gainNode;
	public:
        SampledInstrumentNode(float sampleRate);
        ~SampledInstrumentNode();

		void LoadInstrumentFromJSON(const std::string & jsonStr);

		void NoteOn(ContextRenderLock & r, const int midiNoteNumber, const float amplitude);
		void NoteOff(ContextRenderLock & r, const int midiNoteNumber, const float amplitude);

		void KillAllNotes(); 

		WebCore::GainNode * GetOutputNode() { return gainNode.get(); }
	};

} // LabSound
