// License: BSD 2 Clause
// Copyright (C) 2012, Intel Corporation. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "internal/AudioBus.h"

namespace lab {

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
    input(0)->pull(r, output(0)->bus(r), framesToProcess);
}

void AudioBasicInspectorNode::checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input)
{
    if (input != this->input(0).get())
        return;
    
    unsigned numberOfChannels = input->numberOfChannels(r);

    if (numberOfChannels != output(0)->numberOfChannels()) {
        // This will propagate the channel count to any nodes connected further downstream in the graph.
        output(0)->setNumberOfChannels(r, numberOfChannels);
    }

    AudioNode::checkNumberOfChannelsForInput(r, input);
}

} // namespace lab

