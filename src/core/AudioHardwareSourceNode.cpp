// License: BSD 2 Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioHardwareSourceNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSourceProvider.h"
#include "LabSound/core/AudioBus.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"

namespace lab {

AudioHardwareSourceNode::AudioHardwareSourceNode(const float sampleRate, AudioSourceProvider * audioSourceProvider)
: m_audioSourceProvider(audioSourceProvider)
{
    m_sampleRate = sampleRate; /// @TODO why is sample rate provided when it must come from the context?
    initialize();
}

AudioHardwareSourceNode::~AudioHardwareSourceNode()
{
    uninitialize();
}

void AudioHardwareSourceNode::setFormat(ContextRenderLock & r, uint32_t numberOfChannels, float sourceSampleRate)
{
    if (!output(0))
    {
        addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, numberOfChannels)));
    }

    if (numberOfChannels != m_sourceNumberOfChannels || sourceSampleRate != r.context()->sampleRate())
    {
        // The sample-rate must be equal to the context's sample-rate.
        /// @TODO why is sample rate part of the interface?
        if (!numberOfChannels || numberOfChannels > AudioContext::maxNumberOfChannels || sourceSampleRate != r.context()->sampleRate())
            throw std::runtime_error("AudioHardwareSourceNode must match samplerate of context... ");

        m_sourceNumberOfChannels = numberOfChannels;

        // Do any necesssary re-configuration to the output's number of channels.
        output(0)->setNumberOfChannels(r, numberOfChannels);
    }
}

void AudioHardwareSourceNode::process(ContextRenderLock & r)
{
    if (!output(0))
    {
        setFormat(r, 1, r.context()->sampleRate());
    }
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
        audioSourceProvider()->provideInput(outputBus, r.context()->currentFrames());
    }
    else
    {
        // We failed to acquire the lock.
        outputBus->zero();
    }
}

void AudioHardwareSourceNode::reset(ContextRenderLock & r)
{

}

} // namespace lab
