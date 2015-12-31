// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioBasicProcessorNode_h
#define AudioBasicProcessorNode_h

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioProcessor.h"
#include <memory>

namespace lab {

class AudioBus;
class AudioNodeInput;

// AudioBasicProcessorNode is an AudioNode with one input and one output where the input and output have the same number of channels.
class AudioBasicProcessorNode : public AudioNode 
{

public:

    AudioBasicProcessorNode(float sampleRate);
    virtual ~AudioBasicProcessorNode() {}

    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void pullInputs(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;
    virtual void initialize() override;
    virtual void uninitialize() override;

    // Called in the main thread when the number of channels for the input may have changed.
    virtual void checkNumberOfChannelsForInput(ContextRenderLock&, AudioNodeInput*) override;

    // Returns the number of channels for both the input and the output.
    unsigned numberOfChannels();

protected:

    virtual double tailTime() const override;
    virtual double latencyTime() const override;

    AudioProcessor * processor();

    std::unique_ptr<AudioProcessor> m_processor;

};

} // namespace lab

#endif // AudioBasicProcessorNode_h
