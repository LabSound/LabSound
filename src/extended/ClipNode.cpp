// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioProcessor.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/ClipNode.h"
#include "LabSound/extended/Registry.h"
#include "LabSound/extended/VectorMath.h"

#include <iostream>
#include <vector>

using namespace lab;

namespace lab
{

/////////////////////////////////////
// Private ClipNode Implementation //
/////////////////////////////////////

static char const * const s_ClipModes[ClipNode::Mode::_Count + 1] = {"Clip", "Tanh", nullptr};

const float fMax = std::numeric_limits<float>::max();
static AudioParamDescriptor s_cnParams[] = {
    {"a",    "A   ", -1.0, -fMax, fMax},
    {"b",    "B   ",  1.0, -fMax, fMax}, nullptr};

static AudioSettingDescriptor s_cnSettings[] = {{"mode", "MODE", SettingType::Enum, s_ClipModes}, nullptr};

AudioNodeDescriptor * ClipNode::desc()
{
    static AudioNodeDescriptor d {s_cnParams, s_cnSettings, 0};
    return &d;
}

class ClipNode::ClipNodeInternal : public lab::AudioProcessor
{
public:
    ClipNodeInternal(ClipNode * owner)
        : AudioProcessor()
        , _owner(owner)
    {
    }

    virtual ~ClipNodeInternal() {}

    virtual void initialize() override {}
    virtual void uninitialize() override {}

    // Processes the source to destination bus.
    virtual void process(ContextRenderLock & r,
                         const lab::AudioBus * sourceBus, lab::AudioBus * destinationBus,
                         int framesToProcess) override
    {
        int srcChannels = (int) sourceBus->numberOfChannels();
        int dstChannels = (int) destinationBus->numberOfChannels();
        if (dstChannels != srcChannels)
        {
            _owner->output(0)->setNumberOfChannels(r, srcChannels);
            destinationBus = _owner->output(0)->bus(r); 
            /// @todo no need to pass in the destination bus since owner is retained.
            /// @todo perhaps flatten out AudioProcessor as well, as it's not adding anything particularly
        }

        if (!srcChannels)
        {
            destinationBus->zero();
            return;
        }

        ClipNode::Mode clipMode = static_cast<ClipNode::Mode>(mode->valueUint32());

        if (clipMode == ClipNode::TANH)
        {
            /// @fixme these values should be per sample, not per quantum
            /// -or- they should be settings if they don't vary per sample
            float outputGain = aVal->value();
            float inputGain = bVal->value();

            for (int channelIndex = 0; channelIndex < dstChannels; ++channelIndex)
            {
                int srcIndex = srcChannels < channelIndex ? srcChannels : channelIndex;
                float const * source = sourceBus->channel(srcIndex)->data();
                float * destination = destinationBus->channel(channelIndex)->mutableData();
                if (destination)
                    for (int i = 0; i < framesToProcess; ++i)
                    {
                        *destination++ = outputGain * tanhf(inputGain * source[i]);
                    }
            }
        }
        else
        {
            /// @fixme these values should be per sample, not per quantum
            /// -or- they should be settings if they don't vary per sample
            float minf = aVal->value();
            float maxf = bVal->value();

            for (int channelIndex = 0; channelIndex < dstChannels; ++channelIndex)
            {
                int srcIndex = srcChannels < channelIndex ? srcChannels : channelIndex;
                float const * source = sourceBus->channel(srcIndex)->data();
                float * destination = destinationBus->channel(channelIndex)->mutableData();
                if (destination)
                    for (int i = 0; i < framesToProcess; ++i)
                    {
                        float d = source[i];

                        if (d < minf)
                            d = minf;
                        else if (d > maxf)
                            d = maxf;

                        *destination++ = d;
                    }
            }
        }
    }

    virtual void reset() override {}

    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }

    ClipNode * _owner = nullptr;
    std::shared_ptr<AudioParam> aVal;
    std::shared_ptr<AudioParam> bVal;
    std::shared_ptr<AudioSetting> mode;
};

/////////////////////
// Public ClipNode //
/////////////////////

ClipNode::ClipNode(AudioContext & ac)
: lab::AudioBasicProcessorNode(ac, *desc())
{
    internalNode = new ClipNodeInternal(this);
    internalNode->aVal = param("a");
    internalNode->bVal = param("b");
    internalNode->mode = setting("mode");
    internalNode->mode->setUint32(static_cast<uint32_t>(ClipNode::CLIP));

    m_processor.reset(internalNode);
    initialize();
}

ClipNode::~ClipNode()
{
    uninitialize();
}

void ClipNode::setMode(Mode m)
{
    internalNode->mode->setUint32(uint32_t(m));
}

std::shared_ptr<AudioParam> ClipNode::aVal()
{
    return internalNode->aVal;
}
std::shared_ptr<AudioParam> ClipNode::bVal()
{
    return internalNode->bVal;
}

}  // end namespace lab
