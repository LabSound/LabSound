/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioParam.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioBus.h"
#include "internal/Assertions.h"

#if DEBUG_AUDIONODE_REFERENCES
#include <stdio.h>
#include <map>
#endif

using namespace std;

namespace WebCore {

AudioNode::AudioNode(float sampleRate)
    : m_isInitialized(false)
    , m_nodeType(NodeTypeUnknown)
    , m_sampleRate(sampleRate)
    , m_lastProcessingTime(-1)
    , m_lastNonSilentTime(-1)
    , m_normalRefCount(1) // start out with normal refCount == 1 (like WTF::RefCounted class)
    , m_connectionRefCount(0)
    , m_isMarkedForDeletion(false)
    , m_inputCount(0)
    , m_outputCount(0)
{
#if DEBUG_AUDIONODE_REFERENCES
    if (!s_isNodeCountInitialized) {
        s_isNodeCountInitialized = true;
        atexit(AudioNode::printNodeCounts);
    }
#endif
}

AudioNode::~AudioNode()
{
    /// @TODO the problem is that the AudioNodeOutput retains a pointer to AudioNode even though AudioNode is dead.
    /// AudioNode should disconnect all here, but it can't because it needs context locks.
    /// it can't get context locks because it's a destructor.
    /// This needs an architectural revision
    /// because setting initialized false only works if the memory is not reclaimed before the processing loop is called again.
    /// BAD.
    
    m_isInitialized = false;    // mark in case a dead pointer was retained somewhere so it can be seen while debugging
#if 0
    ContextRenderLock r;
    ContextGraphLock g;
    for (int i = 0; i < AUDIONODE_MAXOUTPUTS; ++i)
        if (auto ptr = m_outputs[i])
            AudioNodeOutput::disconnectAll(g, r, ptr);
#endif
    
#if DEBUG_AUDIONODE_REFERENCES
    --s_nodeCount[nodeType()];
    fprintf(stderr, "%p: %d: AudioNode::~AudioNode() %d %d\n", this, nodeType(), m_normalRefCount.load(), m_connectionRefCount.load());
#endif
}

void AudioNode::initialize()
{
    m_isInitialized = true;
}

void AudioNode::uninitialize()
{
    m_isInitialized = false;
}

void AudioNode::setNodeType(NodeType type)
{
    m_nodeType = type;

#if DEBUG_AUDIONODE_REFERENCES
    ++s_nodeCount[type];
#endif
}

void AudioNode::lazyInitialize()
{
    if (!isInitialized())
        initialize();
}

void AudioNode::addInput(std::shared_ptr<AudioNodeInput> input)
{
    static mutex inputLock;
    lock_guard<mutex> lock(inputLock);
    if (m_inputCount < AUDIONODE_MAXINPUTS) {
        m_inputs[m_inputCount] = input;
        ++m_inputCount;
    }
    else {
        ASSERT(0 == "Too many inputs");
    }
}

void AudioNode::addOutput(std::shared_ptr<AudioNodeOutput> output)
{
    static mutex outputLock;
    lock_guard<mutex> lock(outputLock);
    if (m_outputCount < AUDIONODE_MAXOUTPUTS) {
        m_outputs[m_outputCount] = output;
        ++m_outputCount;
    }
    else {
        ASSERT(0 == "Too many outputs");
    }
}

std::shared_ptr<AudioNodeInput> AudioNode::input(unsigned i)
{
    if (i < AUDIONODE_MAXINPUTS)
        return m_inputs[i];
    return 0;
}

std::shared_ptr<AudioNodeOutput> AudioNode::output(unsigned i)
{
    if (i < AUDIONODE_MAXOUTPUTS)
        return m_outputs[i];
    return 0;
}

void AudioNode::connect(AudioContext* context, AudioNode* destination, unsigned outputIndex, unsigned inputIndex)
{
    if (!context) throw std::invalid_argument("No context specified");
    if (!destination) throw std::invalid_argument("No destination specified");
    if (outputIndex >= numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs");
    if (inputIndex >= destination->numberOfInputs()) throw std::out_of_range("Input index greater than available inputs");

    auto input = destination->input(inputIndex);
    auto output = this->output(outputIndex);
    
    // &&& no need to defer this any more? If so remove connect from context and context from connect param list
    context->connect(input, output);

}

void AudioNode::connect(ContextGraphLock& g, std::shared_ptr<AudioParam> param, unsigned outputIndex)
{
    if (!param) throw std::invalid_argument("No parameter specified");
    if (outputIndex >= numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs");
    
    AudioParam::connect(g, param, this->output(outputIndex));
}

void AudioNode::disconnect(unsigned outputIndex)
{
    if (outputIndex >= numberOfOutputs()) throw std::out_of_range("Output index greater than available outputs");
    
    // &&& can't do this, it's recursive
    // &&& context->disconnect(this->output(outputIndex));
}

    
    
void AudioNode::processIfNecessary(ContextRenderLock& r, size_t framesToProcess)
{
    if (!isInitialized())
        return;
    
    auto ac = r.context();
    if (!ac)
        return;
    
    // Ensure that we only process once per rendering quantum.
    // This handles the "fanout" problem where an output is connected to multiple inputs.
    // The first time we're called during this time slice we process, but after that we don't want to re-process,
    // instead our output(s) will already have the results cached in their bus;
    double currentTime = ac->currentTime();
    if (m_lastProcessingTime != currentTime) {
        m_lastProcessingTime = currentTime; // important to first update this time because of feedback loops in the rendering graph

        pullInputs(r, framesToProcess);

        bool silentInputs = inputsAreSilent(r);
        if (!silentInputs)
            m_lastNonSilentTime = (ac->currentSampleFrame() + framesToProcess) / static_cast<double>(m_sampleRate);

        bool ps = propagatesSilence(r.context()->currentTime());
        if (silentInputs && ps)
            silenceOutputs(r);
        else {
            process(r, framesToProcess);
            unsilenceOutputs(r);
        }
    }
}

void AudioNode::checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input)
{
    ASSERT(r.context());
    for (int i = 0; i < AUDIONODE_MAXINPUTS; ++i) {
        if (m_inputs[i].get() == input) {
            input->updateInternalBus(r);
            break;
        }
    }
}

bool AudioNode::propagatesSilence(double now) const
{
    return m_lastNonSilentTime + latencyTime() + tailTime() < now;
}

void AudioNode::pullInputs(ContextRenderLock& r, size_t framesToProcess)
{
    ASSERT(r.context());
    // Process all of the AudioNodes connected to our inputs.
    for (unsigned i = 0; i < AUDIONODE_MAXINPUTS; ++i)
        if (auto in = input(i))
            in->pull(r, 0, framesToProcess);
}

bool AudioNode::inputsAreSilent(ContextRenderLock& r)
{
    for (unsigned i = 0; i < AUDIONODE_MAXINPUTS; ++i) {
        if (auto in = input(i))
            if (!in->bus(r)->isSilent())
                return false;
    }
    return true;
}

void AudioNode::silenceOutputs(ContextRenderLock& r)
{
    for (unsigned i = 0; i < AUDIONODE_MAXOUTPUTS; ++i)
        if (auto out = output(i))
            out->bus(r)->zero();
}

void AudioNode::unsilenceOutputs(ContextRenderLock& r)
{
    for (unsigned i = 0; i < AUDIONODE_MAXOUTPUTS; ++i)
        if (auto out = output(i))
            out->bus(r)->clearSilentFlag();
}

#if DEBUG_AUDIONODE_REFERENCES

bool AudioNode::s_isNodeCountInitialized = false;
int AudioNode::s_nodeCount[NodeTypeEnd];

void AudioNode::printNodeCounts()
{
    
    std::map<int, std::string> NodeLookupTable =
    {
        { NodeTypeUnknown, "NodeTypeUnknown" },
        { NodeTypeDestination, "NodeTypeDestination" },
        { NodeTypeOscillator, "NodeTypeOscillator" },
        { NodeTypeAudioBufferSource, "NodeTypeAudioBufferSource" },
        { NodeTypeMediaElementAudioSource, "NodeTypeMediaElementAudioSource" },
        { NodeTypeMediaStreamAudioDestination, "NodeTypeMediaStreamAudioDestination" },
        { NodeTypeMediaStreamAudioSource, "NodeTypeMediaStreamAudioSource" },
        { NodeTypeBiquadFilter, "NodeTypeBiquadFilter" },
        { NodeTypePanner, "NodeTypePanner" },
        { NodeTypeConvolver, "NodeTypeConvolver" },
        { NodeTypeDelay, "NodeTypeDelay" },
        { NodeTypeGain, "NodeTypeGain" },
        { NodeTypeChannelSplitter, "NodeTypeChannelSplitter" },
        { NodeTypeChannelMerger, "NodeTypeChannelMerger" },
        { NodeTypeAnalyser, "NodeTypeAnalyser" },
        { NodeTypeDynamicsCompressor, "NodeTypeDynamicsCompressor" },
        { NodeTypeWaveShaper, "NodeTypeWaveShaper" },
        { NodeTypeEnd, "NodeTypeEnd" },
    };

    
    fprintf(stderr, "\n\n");
    fprintf(stderr, "===========================\n");
    fprintf(stderr, "AudioNode: reference counts\n");
    fprintf(stderr, "===========================\n");

    for (unsigned i = 0; i < NodeTypeEnd; ++i)
        fprintf(stderr, "%d\tof \t%s \n", s_nodeCount[i], NodeLookupTable[i].c_str());

    fprintf(stderr, "===========================\n\n\n");
}

#endif // DEBUG_AUDIONODE_REFERENCES

} // namespace WebCore
