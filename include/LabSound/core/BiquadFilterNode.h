// License: BSD 2 Clause
// Copyright (C) 2011, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef BiquadFilterNode_h
#define BiquadFilterNode_h

#include "LabSound/core/AudioBasicProcessorNode.h"

namespace lab {

class AudioParam;
class BiquadProcessor;

class BiquadFilterNode : public AudioBasicProcessorNode {
public:
    // These must be defined as in the .idl file and must match those in the BiquadProcessor class.
    enum {
        LOWPASS = 0,
        HIGHPASS = 1,
        BANDPASS = 2,
        LOWSHELF = 3,
        HIGHSHELF = 4,
        PEAKING = 5,
        NOTCH = 6,
        ALLPASS = 7
    };
    
    BiquadFilterNode(float sampleRate);
    
    unsigned short type();
    void setType(unsigned short type);

    std::shared_ptr<AudioParam> frequency();
    std::shared_ptr<AudioParam> q();
    std::shared_ptr<AudioParam> gain();
    std::shared_ptr<AudioParam> detune();

    // Get the magnitude and phase response of the filter at the given
    // set of frequencies (in Hz). The phase response is in radians.
    void getFrequencyResponse(ContextRenderLock&,
                              const std::vector<float>& frequencyHz,
                              std::vector<float>& magResponse,
                              std::vector<float>& phaseResponse);

private:

    BiquadProcessor * biquadProcessor();
};

} // namespace lab

#endif // BiquadFilterNode_h
