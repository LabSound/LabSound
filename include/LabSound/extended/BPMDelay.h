// Copyright (c) 2015 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#ifndef BPM_DELAY_NODE_H
#define BPM_DELAY_NODE_H

#include "LabSound/core/AudioBasicProcessorNode.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/DelayNode.h"

namespace LabSound 
{

	enum class TempoSync : int
	{
		TS_32 = 0,
		TS_16T = 1,
		TS_32D = 2,
		TS_16 = 3,
		TS_8T = 4,
		TS_16D = 5,
		TS_8 = 6,
		TS_4T = 7,
		TS_8D = 8,
		TS_4 = 9,
		TS_2T = 10,
		TS_4D = 11,
		TS_2 = 12,
		TS_2D = 13,
	};

	class BPMDelay : public DelayNode 
	{
		float tempo;
		int noteDivision; 

		void recomputeDelay()
		{
			float dT = float(60.0f * noteDivision) / tempo;
			delayTime()->setValue(dT);
		}

		const std::vector<float> times = 
		{
			1.f / 8.f,
			(1.f / 4.f) * 2.f / 3.f,
			(1.f / 8.f) * 3,f / 2.f,
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
			
	public:

        BPMDelay(float sampleRate);

		virtual ~BPMDelay();

		void UpdateTempo(float newTempo)
		{
			tempo = newTempo;	
			recomputeDelay();
		}

		void SetDelayIndex(TempoSync value)
		{
			if (value <= TempoSync::TS_32 && value >= TempoSync::TS_2D)
			{
				noteDivision = times[static_cast<int>(value)];
				recomputeDelay(); 
			}
			else 
				throw std::invalid_argument("Delay index out of bounds");
		}
	};

}

#endif
