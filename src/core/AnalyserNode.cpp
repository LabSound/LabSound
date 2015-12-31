// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include "internal/AudioBus.h"

namespace lab {

AnalyserNode::AnalyserNode(float sampleRate, size_t fftSize) : AudioBasicInspectorNode(sampleRate, 2), m_analyser(fftSize)
{
    //N.B.: inputs and outputs added by AudioBasicInspectorNode... no need to create here.
    setNodeType(NodeTypeAnalyser);
    initialize();
}

AnalyserNode::~AnalyserNode()
{
    uninitialize();
}

void AnalyserNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    AudioBus* outputBus = output(0)->bus(r);

    if (!isInitialized() || !input(0)->isConnected())
    {
        outputBus->zero();
        return;
    }

    AudioBus* inputBus = input(0)->bus(r);
    
    // Give the analyser the audio which is passing through this AudioNode.
    m_analyser.writeInput(r, inputBus, framesToProcess);

    // For in-place processing, our override of pullInputs() will just pass the audio data through unchanged if the channel count matches from input to output
    // (resulting in inputBus == outputBus). Otherwise, do an up-mix to stereo.
    if (inputBus != outputBus)
    {
        outputBus->copyFrom(*inputBus);
    }
}

void AnalyserNode::reset(ContextRenderLock&)
{
    m_analyser.reset();
}
    
} // namespace lab
