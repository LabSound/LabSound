/*
 * Copyright (C) 2012, Intel Corporation. All rights reserved.
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


#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "internal/AudioBus.h"

namespace WebCore {

AudioBasicInspectorNode::AudioBasicInspectorNode(float sampleRate, int outputChannelCount)
    : AudioNode(sampleRate)
{
    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, outputChannelCount)));
    initialize();
}

// We override pullInputs() as an optimization allowing this node to take advantage of in-place processing,
// where the input is simply passed through unprocessed to the output.
// Note: this only applies if the input and output channel counts match.
void AudioBasicInspectorNode::pullInputs(ContextRenderLock& r, size_t framesToProcess)
{
    // Render input stream - try to render directly into output bus for pass-through processing where process() doesn't need to do anything...
    input(0)->pull(r, output(0)->bus(), framesToProcess);
}

void AudioBasicInspectorNode::checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input)
{
    if (input != this->input(0).get())
        return;
    
    unsigned numberOfChannels = input->numberOfChannels();

    if (numberOfChannels != output(0)->numberOfChannels()) {
        // This will propagate the channel count to any nodes connected further downstream in the graph.
        output(0)->setNumberOfChannels(r, numberOfChannels);
    }

    AudioNode::checkNumberOfChannelsForInput(r, input);
}

} // namespace WebCore

