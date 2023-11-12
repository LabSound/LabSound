// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2013 Nick Porcino, All rights reserved.

#include "LabSound/extended/PowerMonitorNode.h"
#include "LabSound/extended/Registry.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"

namespace lab
{

using namespace lab;

static AudioSettingDescriptor s_pmSettings[] = {{"windowSize", "WNSZ", SettingType::Integer}, nullptr};

AudioNodeDescriptor * PowerMonitorNode::desc()
{
    static AudioNodeDescriptor d {nullptr, s_pmSettings, 0};
    return &d;
}

PowerMonitorNode::PowerMonitorNode(AudioContext & ac)
    : AudioBasicInspectorNode(ac, *desc())
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
    // This node acts as a pass-through if it is embedded in a chain

    AudioBus * outputBus = output(0)->bus(r);

    if (!isInitialized() || !input(0)->isConnected())
    {
        if (outputBus)
            outputBus->zero();
        return;
    }

    AudioBus * bus = input(0)->bus(r);
    bool isBusGood = bus && bus->numberOfChannels() > 0 && bus->channel(0)->length() >= bufferSize;
    if (!isBusGood)
    {
        outputBus->zero();
        return;
    }

    // calculate the power of this buffer
    {
        int start = static_cast<int>(bufferSize) - static_cast<int>(_windowSize->valueUint32());
        int end = static_cast<int>(bufferSize);
        if (start < 0)
            start = 0;

        float power = 0;
        int numberOfChannels = bus->numberOfChannels();
        for (int c = 0; c < numberOfChannels; ++c) {
            const float* data = bus->channel(c)->data();
            for (int i = start; i < end; ++i)
            {
                float p = data[i];
                power += p * p;
            }
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

    if (bus != outputBus)
        outputBus->copyFrom(*bus);
}

void PowerMonitorNode::reset(ContextRenderLock &)
{
    _db = 0;
}

}  // namespace lab
