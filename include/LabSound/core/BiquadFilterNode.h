// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef BiquadFilterNode_h
#define BiquadFilterNode_h

#include "LabSound/core/AudioBasicProcessorNode.h"

namespace lab {

class AudioParam;
class BiquadProcessor;

class BiquadFilterNode : public AudioBasicProcessorNode 
{

    BiquadProcessor * biquadProcessor();
    BiquadProcessor * biquadProcessor() const;
    std::shared_ptr<AudioSetting> m_type;

public:
    
    BiquadFilterNode();
    
    FilterType type() const;
    void setType(FilterType type);

    std::shared_ptr<AudioParam> frequency();
    std::shared_ptr<AudioParam> q();
    std::shared_ptr<AudioParam> gain();
    std::shared_ptr<AudioParam> detune();

    // Get the magnitude and phase response of the filter at the given
    // set of frequencies (in Hz). The phase response is in radians.
    void getFrequencyResponse(ContextRenderLock&, const std::vector<float>& frequencyHz, std::vector<float>& magResponse, std::vector<float>& phaseResponse);

};

} // namespace lab

#endif // BiquadFilterNode_h
