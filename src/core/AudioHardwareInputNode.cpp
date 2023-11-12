// SPDX-License-Identifier: BSD-3-Clause
// Copyright (C) 2012, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioHardwareInputNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSourceProvider.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Logging.h"
#include "LabSound/extended/Registry.h"

#include <memory>

namespace lab
{

AudioNodeDescriptor * AudioHardwareInputNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr, 1};
    return &d;
}

AudioHardwareInputNode::AudioHardwareInputNode(AudioContext & ac,
                                               AudioSourceProvider * audioSourceProvider)
: AudioNode(ac, *desc())
, m_audioSourceProvider(audioSourceProvider)
{
    initialize();
}


AudioHardwareInputNode::~AudioHardwareInputNode()
{
    uninitialize();
}

void AudioHardwareInputNode::process(ContextRenderLock &r, int bufferSize)
{
    AudioBus * outputBus = output(0)->bus(r);

    // This used to be the function of a manual call to setFormat()
    if (m_sourceNumberOfChannels == 0)
    {
        auto destinationNode = r.context()->destinationNode();
        auto device = destinationNode->device();
        auto inputConfig = device->getInputConfig();
        m_sourceNumberOfChannels = inputConfig.desired_channels;
        output(0)->setNumberOfChannels(r, m_sourceNumberOfChannels);  // Reconfigure the output's number of channels.
        outputBus = output(0)->bus(r);  // outputBus pointer was invalidated
    }

    if (!m_audioSourceProvider)
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
        // provide entire buffer
        m_audioSourceProvider->provideInput(outputBus, bufferSize);
    }
    else
    {
        // We failed to acquire the lock.
        outputBus->zero();
    }
}

void AudioHardwareInputNode::reset(ContextRenderLock & r)
{
}

}  // namespace lab
