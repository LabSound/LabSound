/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LabSound/core/ChannelMergerNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "internal/Assertions.h"
#include "internal/AudioBus.h"

using namespace std;

namespace WebCore {

const unsigned DefaultNumberOfOutputChannels = 1;

ChannelMergerNode::ChannelMergerNode(float sampleRate, unsigned numberOfInputs)
    : AudioNode(sampleRate)
    , m_desiredNumberOfOutputChannels(DefaultNumberOfOutputChannels)
{
    numberOfInputs = std::max(1U, std::min(numberOfInputs, AudioContext::maxNumberOfChannels));
    
    // Create the requested number of inputs.
    for (unsigned i = 0; i < numberOfInputs; ++i)
        addInput(unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));

    addOutput(unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 1)));
    
    setNodeType(NodeTypeChannelMerger);
    
    initialize();
}

void ChannelMergerNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    auto output = this->output(0);
    ASSERT_UNUSED(framesToProcess, framesToProcess == output->bus(r)->length());

    // Output bus not updated yet, so just output silence. See Note * in checkNumberOfChannelsForInput
    if (m_desiredNumberOfOutputChannels != output->numberOfChannels()) {
        output->bus(r)->zero();
        return;
    }
    
    // Merge all the channels from all the inputs into one output.
    unsigned outputChannelIndex = 0;
    for (unsigned i = 0; i < numberOfInputs(); ++i) {
        auto input = this->input(i);
        if (input->isConnected()) {
            unsigned numberOfInputChannels = input->bus(r)->numberOfChannels();
            
            // Merge channels from this particular input.
            for (unsigned j = 0; j < numberOfInputChannels; ++j) {
                AudioChannel* inputChannel = input->bus(r)->channel(j);
                AudioChannel* outputChannel = output->bus(r)->channel(outputChannelIndex);
                outputChannel->copyFrom(inputChannel);
                
                ++outputChannelIndex;
            }
        }
    }
    
    ASSERT(outputChannelIndex == output->numberOfChannels());
}

void ChannelMergerNode::reset(ContextRenderLock&)
{
}

// Any time a connection or disconnection happens on any of our inputs, we potentially need to change the
// number of channels of our output.
void ChannelMergerNode::checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input)
{
    // Count how many channels we have all together from all of the inputs.
    unsigned numberOfOutputChannels = 0;
    for (unsigned i = 0; i < numberOfInputs(); ++i) {
        auto input = this->input(i);
        
        if (input->isConnected())
            numberOfOutputChannels += input->bus(r)->numberOfChannels();
    }

    // Set the correct number of channels on the output
    auto output = this->output(0);
    output->setNumberOfChannels(r, numberOfOutputChannels);
    // Note * There can in rare cases be a slight delay before the output bus is updated to the new number of
    // channels because of tryLocks() in the context's updating system. So record the new number of
    // output channels here.
    m_desiredNumberOfOutputChannels = numberOfOutputChannels;

    AudioNode::checkNumberOfChannelsForInput(r, input);
}

} // namespace WebCore
