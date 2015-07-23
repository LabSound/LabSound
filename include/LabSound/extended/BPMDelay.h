// Copyright (c) 2015 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#ifndef BPM_DELAY_NODE_H
#define BPM_DELAY_NODE_H

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/DelayNode.h"

// Debug
#include <iostream>

namespace LabSound 
{
	enum TempoSync
	{
		TS_32,
		TS_16T,
		TS_32D,
		TS_16,
		TS_8T,
		TS_16D,
		TS_8,
		TS_4T,
		TS_8D,
		TS_4,
		TS_2T,
		TS_4D,
		TS_2,
		TS_2D,
	};

	class BPMDelay : public WebCore::DelayNode 
	{
		float tempo;
		int noteDivision; 
		std::vector<float> times;

		void recomputeDelay()
		{
			float dT = float(60.0f * noteDivision) / tempo;
			delayTime()->setValue(dT);
		}
			
	public:

        BPMDelay(float sampleRate, float tempo);

		virtual ~BPMDelay();

		void UpdateTempo(float newTempo)
		{
			tempo = newTempo;	
			recomputeDelay();
		}

		void SetDelayIndex(TempoSync value);

	};
}

#endif
