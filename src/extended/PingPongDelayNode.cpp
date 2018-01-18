// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/Macros.h"

#include "LabSound/extended/PingPongDelayNode.h"
#include "LabSound/extended/AudioContextLock.h"

#include <algorithm>

using namespace lab;

namespace lab 
{
    PingPongDelayNode::PingPongDelayNode(float sampleRate, float tempo)
    {
        input = std::make_shared<lab::GainNode>();
        output = std::make_shared<lab::GainNode>();

        leftDelay = std::make_shared<lab::BPMDelay>(sampleRate, tempo);
        rightDelay = std::make_shared<lab::BPMDelay>(sampleRate, tempo);

        splitterGain = std::make_shared<lab::GainNode>();
        wetGain = std::make_shared<lab::GainNode>();
        feedbackGain = std::make_shared<lab::GainNode>();

        merger = std::make_shared<lab::ChannelMergerNode>(2);
        splitter = std::make_shared<lab::ChannelSplitterNode>(2);

        SetDelayIndex(TempoSync::TS_8);
        SetFeedback(0.5f);
        SetLevel(1.0f);
    }
    
    PingPongDelayNode::~PingPongDelayNode()
    {

    }

    void PingPongDelayNode::SetTempo(float t)
    { 
        tempo = t; 
        leftDelay->SetTempo(tempo);
        rightDelay->SetTempo(tempo);
    }

    void PingPongDelayNode::SetFeedback(float f) 
    {
        auto clamped = clampTo<float>(f,0.0f, 1.0f);
        feedbackGain->gain()->setValue(clamped);
    }

    void PingPongDelayNode::SetLevel(float f) 
    {
        auto clamped = clampTo<float>(f,0.0f, 1.0f);
        wetGain->gain()->setValue(clamped);
    }

    void PingPongDelayNode::SetDelayIndex(TempoSync value)
    {
        leftDelay->SetDelayIndex(value);
        rightDelay->SetDelayIndex(value);
    }

    void PingPongDelayNode::BuildSubgraph(std::unique_ptr<AudioContext> & ac) 
    {

        // Input into splitter
        ac->connect(splitter, input, 0, 0);

        ac->connect(splitterGain, splitter, 0, 0);
        ac->connect(splitterGain, splitter, 1, 0);

        ac->connect(wetGain, splitterGain, 0, 0);
        splitterGain->gain()->setValue(0.5f);

        ac->connect(leftDelay, wetGain, 0, 0);
        ac->connect(leftDelay, feedbackGain, 0, 0);

        ac->connect(rightDelay, leftDelay, 0, 0);
        ac->connect(feedbackGain, rightDelay, 0, 0);

        ac->connect(merger, leftDelay, 0, 0);
        ac->connect(merger, rightDelay, 0, 1);

        ac->connect(output, merger, 0, 0);

        // Activate with input->output
        ac->connect(output, input, 0, 0);
    }


}
