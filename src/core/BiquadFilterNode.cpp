// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/BiquadFilterNode.h"

#include "LabSound/core/AudioSetting.h"
#include "internal/BiquadProcessor.h"

namespace lab
{

BiquadFilterNode::BiquadFilterNode() : AudioBasicProcessorNode()
, m_type(std::make_shared<AudioSetting>("type"))
{
    // Initially setup as lowpass filter.
    m_processor.reset(new BiquadProcessor(1, false));

    m_params.push_back(biquadProcessor()->parameter1());
    m_params.push_back(biquadProcessor()->parameter2());
    m_params.push_back(biquadProcessor()->parameter3());
    m_params.push_back(biquadProcessor()->parameter4());


    m_type->setValueChanged(
        [this]()
        {
            uint32_t type = m_type->valueUint32();
            if (type > static_cast<uint32_t>(FilterType::ALLPASS)) 
                throw std::out_of_range("Filter type exceeds index of known types");

            biquadProcessor()->setType(static_cast<FilterType>(type));
        }
    );
    m_settings.push_back(m_type);

    initialize();
}

void BiquadFilterNode::setType(FilterType type)
{
    m_type->setUint32(uint32_t(type));
}


void BiquadFilterNode::getFrequencyResponse(ContextRenderLock& r,
                                            const std::vector<float>& frequencyHz,
                                            std::vector<float>& magResponse,
                                            std::vector<float>& phaseResponse)
{
    if (!frequencyHz.size() || !magResponse.size() || !phaseResponse.size())
        return;

    size_t n = std::min(frequencyHz.size(),
                     std::min(magResponse.size(), phaseResponse.size()));

    if (n) 
    {
        biquadProcessor()->getFrequencyResponse(r, n, &frequencyHz[0], &magResponse[0], &phaseResponse[0]);
    }
}

BiquadProcessor * BiquadFilterNode::biquadProcessor()
{
    return static_cast<BiquadProcessor*>(processor());
}

BiquadProcessor * BiquadFilterNode::biquadProcessor() const
{
    return static_cast<BiquadProcessor *>(processor());
}

FilterType BiquadFilterNode::type() const
{
    return biquadProcessor()->type();
}

std::shared_ptr<AudioParam> BiquadFilterNode::frequency()
{
    return biquadProcessor()->parameter1();
}

std::shared_ptr<AudioParam> BiquadFilterNode::q()
{
    return biquadProcessor()->parameter2();
}
std::shared_ptr<AudioParam> BiquadFilterNode::gain()
{
    return biquadProcessor()->parameter3();
}

std::shared_ptr<AudioParam> BiquadFilterNode::detune()
{
    return biquadProcessor()->parameter4();
}

} // namespace lab
