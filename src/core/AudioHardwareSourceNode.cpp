// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioHardwareSourceNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSourceProvider.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"

#include "internal/AudioBus.h"

namespace lab {

AudioHardwareSourceNode::AudioHardwareSourceNode(AudioSourceProvider * audioSourceProvider, float sampleRate) : AudioSourceNode(sampleRate)
, m_audioSourceProvider(audioSourceProvider)
, m_sourceNumberOfChannels(0)
{
    
    // @tofix - defaults to stereo. will change when this node eventually supports multi-channel audio
    addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 2)));

    setNodeType(lab::NodeType::NodeTypeHardwareSource);

    initialize();
}

AudioHardwareSourceNode::~AudioHardwareSourceNode()
{
    uninitialize();
}

void AudioHardwareSourceNode::setFormat(ContextRenderLock & r, size_t numberOfChannels, float sourceSampleRate)
{
    if (numberOfChannels != m_sourceNumberOfChannels || sourceSampleRate != sampleRate())
    {
        // The sample-rate must be equal to the context's sample-rate.
        if (!numberOfChannels || numberOfChannels > AudioContext::maxNumberOfChannels || sourceSampleRate != sampleRate())
            throw std::runtime_error("AudioHardwareSourceNode must match samplerate of context... ");

        m_sourceNumberOfChannels = numberOfChannels;
        
        // Do any necesssary re-configuration to the output's number of channels.
        output(0)->setNumberOfChannels(r, numberOfChannels);
    }
}

void AudioHardwareSourceNode::process(ContextRenderLock& r, size_t numberOfFrames)
{
    AudioBus * outputBus = output(0)->bus(r);

    if (!audioSourceProvider())
    {
        outputBus->zero();
        return;
    }

    if (m_sourceNumberOfChannels != outputBus->numberOfChannels())
    {
        outputBus->zero();
        return;
    }

    // Use a tryLock() to avoid contention in the real-time audio thread.
    // If we fail to acquire the lock then the MediaStream must be in the middle of
    // a format change, so we output silence in this case.
    if (r.context())
    {
        audioSourceProvider()->provideInput(outputBus, numberOfFrames);
    }
    else
    {
        // We failed to acquire the lock.
        outputBus->zero();
    }
}

void AudioHardwareSourceNode::reset(ContextRenderLock&)
{
    
}

} // namespace lab
