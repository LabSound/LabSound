// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef PING_PONG_DELAY_NODE_H
#define PING_PONG_DELAY_NODE_H

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/GainNode.h"
#include "LabSound/core/ChannelSplitterNode.h"
#include "LabSound/core/ChannelMergerNode.h"

#include "LabSound/extended/BPMDelay.h"

namespace lab 
{
    class ContextGraphLock;

    class SubgraphNode
    {
    public:
        std::shared_ptr<GainNode> output;
        std::shared_ptr<GainNode> input;
        virtual void BuildSubgraph(ContextGraphLock & lock) = 0;
        virtual ~SubgraphNode() { }
    };

    class PingPongDelayNode : public SubgraphNode
    {
        float tempo;
        int noteDivision;

        std::shared_ptr<BPMDelay> leftDelay;
        std::shared_ptr<BPMDelay> rightDelay;

        std::shared_ptr<GainNode> splitterGain;
        std::shared_ptr<GainNode> wetGain;
        std::shared_ptr<GainNode> feedbackGain;

        std::shared_ptr<ChannelMergerNode> merger;
        std::shared_ptr<ChannelSplitterNode> splitter;

    public:

        PingPongDelayNode(float sampleRate, float tempo);
        virtual ~PingPongDelayNode();

        void SetTempo(float t);
        void SetFeedback(float f);
        void SetLevel(float f);
        void SetDelayIndex(TempoSync value);

        virtual void BuildSubgraph(ContextGraphLock & lock) override;
    };
}

#endif
