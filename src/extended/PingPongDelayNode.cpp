// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"

#include "LabSound/extended/PingPongDelayNode.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioBus.h"

#include <algorithm>
#include <WTF/MathExtras.h>

using namespace lab;

namespace lab 
{
    PingPongDelayNode::PingPongDelayNode(float sampleRate, float tempo)
    {
        input = std::make_shared<lab::GainNode>(sampleRate);
        output = std::make_shared<lab::GainNode>(sampleRate);

        leftDelay = std::make_shared<lab::BPMDelay>(sampleRate, tempo);
        rightDelay = std::make_shared<lab::BPMDelay>(sampleRate, tempo);

        splitterGain = std::make_shared<lab::GainNode>(sampleRate);
        wetGain = std::make_shared<lab::GainNode>(sampleRate);
        feedbackGain = std::make_shared<lab::GainNode>(sampleRate);

        merger = std::make_shared<lab::ChannelMergerNode>(sampleRate, 2);
        splitter = std::make_shared<lab::ChannelSplitterNode>(sampleRate, 2);

        SetDelayIndex(TempoSync::TS_8);
        SetFeedback(0.5f);
        SetLevel(0.5f);
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

    void PingPongDelayNode::BuildSubgraph(ContextGraphLock & lock) 
    {
        auto ac = lock.context();

        if (!ac) 
            throw std::invalid_argument("Graph lock could not acquire context");

        // Input into splitter
        input->connect(ac, splitter.get(), 0, 0);

        splitter->connect(ac, splitterGain.get(), 0, 0);
        splitter->connect(ac, splitterGain.get(), 1, 0);

        splitterGain->connect(ac, wetGain.get(), 0, 0); 
        splitterGain->gain()->setValue(0.5f);

        wetGain->connect(ac, leftDelay.get(), 0, 0);

        feedbackGain->connect(ac, leftDelay.get(), 0, 0);

        leftDelay->connect(ac, rightDelay.get(), 0, 0);
        rightDelay->connect(ac, feedbackGain.get(), 0, 0);

        leftDelay->connect(ac, merger.get(), 0, 0);
        rightDelay->connect(ac, merger.get(), 0, 1);

        merger->connect(ac, output.get(), 0, 0);

        // Activate with input->output
        input->connect(ac, output.get(), 0, 0);
    }


}
