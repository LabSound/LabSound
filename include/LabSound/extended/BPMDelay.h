// Copyright (c) 2015 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#ifndef BPM_DELAY_NODE_H
#define BPM_DELAY_NODE_H

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/DelayNode.h"

namespace lab 
{
	class BPMDelay : public DelayNode 
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

		void SetTempo(float newTempo)
		{
			tempo = newTempo;	
			recomputeDelay();
		}

		void SetDelayIndex(TempoSync value);
	};
}

#endif
