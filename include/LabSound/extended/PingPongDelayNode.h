// Copyright (c) 2015 Dimitri Diakopoulos, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#ifndef PING_PONG_DELAY_NODE_H
#define PING_PONG_DELAY_NODE_H

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/GainNode.h"
#include "LabSound/core/ChannelSplitterNode.h"
#include "LabSound/core/ChannelMergerNode.h"

#include "LabSound/extended/BPMDelay.h"




namespace LabSound 
{

}


namespace LabSound 
{
	class ContextGraphLock;

	class SubgraphNode
	{
	public:
		std::shared_ptr<WebCore::GainNode> output;
		std::shared_ptr<WebCore::GainNode> input;
		virtual void BuildSubgraph(ContextGraphLock & lock) = 0;
        virtual ~SubgraphNode() { }
	};

	class PingPongDelayNode : public SubgraphNode
	{
		float tempo;
		int noteDivision;

		std::shared_ptr<LabSound::BPMDelay> leftDelay;
		std::shared_ptr<LabSound::BPMDelay> rightDelay;

		std::shared_ptr<WebCore::GainNode> splitterGain;
		std::shared_ptr<WebCore::GainNode> wetGain;
		std::shared_ptr<WebCore::GainNode> feedbackGain;

		std::shared_ptr<WebCore::ChannelMergerNode> merger;
		std::shared_ptr<WebCore::ChannelSplitterNode> splitter;

	public:

        PingPongDelayNode(float sampleRate, float tempo);
		virtual ~PingPongDelayNode();

		void SetTempo(float t);
		void SetFeedback(float f);
		void SetLevel(float f);
		void SetDelayIndex(WebCore::TempoSync value);

		virtual void BuildSubgraph(LabSound::ContextGraphLock & lock) override;
	};
}

#endif
