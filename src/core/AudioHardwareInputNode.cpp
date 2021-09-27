// SPDX-License-Identifier: BSD-3-Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioHardwareInputNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioSourceProvider.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"
#include "LabSound/extended/Registry.h"

#include <memory>

namespace lab
{

AudioNodeDescriptor * AudioHardwareInputNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr};
    return &d;
}

AudioHardwareInputNode::AudioHardwareInputNode(AudioContext & ac, AudioSourceProvider * audioSourceProvider)
    : AudioNode(ac, *desc())
    , m_audioSourceProvider(audioSourceProvider)
{
    addOutput("out", 1, AudioNode::ProcessingSizeInFrames);
    initialize();
}


AudioHardwareInputNode::~AudioHardwareInputNode()
{
    uninitialize();
}

void AudioHardwareInputNode::process(ContextRenderLock &r, int bufferSize)
{
    AudioBus * dstBus = outputBus(r, 0);

    // This used to be the function of a manual call to setFormat()
    if (m_sourceNumberOfChannels == 0)
    {
        auto DeviceAsAudioNode = r.context()->device();

        if (auto DeviceAsRenderCallback = std::dynamic_pointer_cast<AudioDeviceRenderCallback>(DeviceAsAudioNode))
        {
            auto inputConfig = DeviceAsRenderCallback->getInputConfig();
            m_sourceNumberOfChannels = inputConfig.desired_channels;
            auto bus = outputBus(r, 0);
            bus->setNumberOfChannels(r, m_sourceNumberOfChannels);
        }
    }

    if (!m_audioSourceProvider)
    {
        dstBus->zero();
        return;
    }

    if (m_sourceNumberOfChannels != dstBus->numberOfChannels())
    {
        dstBus->zero();
        return;
    }

    // Use a tryLock() to avoid contention in the real-time audio thread.
    // If we fail to acquire the lock then the MediaStream must be in the middle of
    // a format change, so we output silence in this case.
    if (r.context())
    {
        // provide entire buffer
        m_audioSourceProvider->provideInput(dstBus, bufferSize);
    }
    else
    {
        // We failed to acquire the lock.
        dstBus->zero();
    }
}

void AudioHardwareInputNode::reset(ContextRenderLock & r)
{
}

}  // namespace lab
