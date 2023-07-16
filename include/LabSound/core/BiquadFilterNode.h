// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef BiquadFilterNode_h
#define BiquadFilterNode_h

#include "LabSound/core/AudioBasicProcessorNode.h"

namespace lab
{

class AudioParam;
class Biquad;

class BiquadFilterNode : public AudioBasicProcessorNode
{
    class BiquadFilterNodeInternal;
    BiquadFilterNodeInternal * biquad_impl;

public:
    BiquadFilterNode(AudioContext& ac);
    virtual ~BiquadFilterNode();

    static const char* static_name() { return "BiquadFilter"; }
    virtual const char* name() const override { return static_name(); }
    static AudioNodeDescriptor * desc();

    FilterType type() const;
    void setType(FilterType type);

    std::shared_ptr<AudioParam> frequency();
    std::shared_ptr<AudioParam> q();
    std::shared_ptr<AudioParam> gain();
    std::shared_ptr<AudioParam> detune();

    /*
     https://developer.mozilla.org/en-US/docs/Web/API/BiquadFilterNode/getFrequencyResponse
     The getFrequencyResponse() method of the BiquadFilterNode interface takes
     the current filtering algorithm's settings and calculates the frequency
     response for frequencies specified in a specified array of frequencies.

     The two output arrays, magResponseOutput and phaseResponseOutput, must be
     created before calling this method; they must be the same size as the
     array of input frequency values (frequencyArray).


     Get the magnitude and phase response of the filter at the given
     set of frequencies (in Hz). The phase response is in radians.
     */
    void getFrequencyResponse(ContextRenderLock &, const std::vector<float> & frequencyHz, std::vector<float> & magResponse, std::vector<float> & phaseResponse);
};

}  // namespace lab

#endif  // BiquadFilterNode_h
