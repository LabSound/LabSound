// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2013 Nick Porcino, All rights reserved.

#include "LabSound/extended/PowerMonitorNode.h"
#include "LabSound/extended/Registry.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"

namespace lab
{

using namespace lab;

static AudioSettingDescriptor s_pmSettings[] = {{"windowSize", "WNSZ", SettingType::Integer}, nullptr};

AudioNodeDescriptor * PowerMonitorNode::desc()
{
    static AudioNodeDescriptor d {nullptr, s_pmSettings};
    return &d;
}

PowerMonitorNode::PowerMonitorNode(AudioContext & ac)
    : AudioBasicInspectorNode(ac, *desc(), 2)
    , _db(0)
{
    _windowSize = setting("windowSize");
    _windowSize->setUint32(128);
    initialize();
}

PowerMonitorNode::~PowerMonitorNode()
{
    uninitialize();
}

void PowerMonitorNode::windowSize(int ws)
{
    _windowSize->setUint32(static_cast<uint32_t>(ws));
}

int PowerMonitorNode::windowSize() const
{
    return _windowSize->valueUint32();
}

void PowerMonitorNode::process(ContextRenderLock & r, int bufferSize)
{
    // deal with the output in case the power monitor node is embedded in a signal chain for some reason.
    // It's merely a pass through though.

    AudioBus * dstBus = outputBus(r);
    AudioBus * srcBus = inputBus(r, 0);

    if (!dstBus || !srcBus)
    {
        if (dstBus)
            dstBus->zero();
        return;
    }

    bool isBusGood = srcBus && srcBus->numberOfChannels() > 0 && srcBus->channel(0)->length() >= bufferSize;
    if (!isBusGood)
    {
        dstBus->zero();
        return;
    }

    // specific to this node
    {
        std::vector<const float *> channels;
        int numberOfChannels = srcBus->numberOfChannels();
        for (int i = 0; i < numberOfChannels; ++i)
        {
            channels.push_back(srcBus->channel(i)->data());
        }

        int start = static_cast<int>(bufferSize) - static_cast<int>(_windowSize->valueUint32());
        int end = static_cast<int>(bufferSize);
        if (start < 0)
            start = 0;

        float power = 0;
        for (int c = 0; c < numberOfChannels; ++c)
            for (int i = start; i < end; ++i)
            {
                float p = channels[c][i];
                power += p * p;
            }
        float rms = sqrtf(power / (numberOfChannels * bufferSize));

        // Protect against accidental overload due to bad values in input stream
        const float kMinPower = 0.000125f;
        if (std::isinf(power) || std::isnan(power) || power < kMinPower)
            power = kMinPower;

        // db is 20 * log10(rms/Vref) where Vref is 1.0
        _db = 20.0f * logf(rms) / logf(10.0f);
    }
    // to here

    if (srcBus != dstBus)
        dstBus->copyFrom(*srcBus);
}

void PowerMonitorNode::reset(ContextRenderLock &)
{
    _db = 0;
}

}  // namespace lab
