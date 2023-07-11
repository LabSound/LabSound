
// License: BSD 3 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioParam_h
#define AudioParam_h

#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/core/AudioParamDescriptor.h"

#include <string>
#include <vector>

namespace lab {

struct AudioParamKnot { float t, v; };

/// @TODO overriding input should instead be a summing bus

class AudioParam
{
    const AudioParamDescriptor* _descriptor;
    std::shared_ptr<AudioNode> _overridingInput;
    std::shared_ptr<AudioBus> _bus;
    float _intrinsicValue;
    int _overridingChannel;
    std::vector<AudioParamKnot> _knots;
    
public:
    AudioParam(AudioParamDescriptor const * const desc) noexcept;
    virtual ~AudioParam() = default;
    AudioParam() = delete;
    
    // updates the values in the internal bus; either computes values per
    // the _knots vector, or propagates the overriding input's output.
    void process(ContextRenderLock &, int bufferSize);
    
    std::string name() const { return _descriptor->name; }
    std::string shortName() const { return _descriptor->shortName; }
    
    float minValue() const { return _descriptor->minValue; }
    float maxValue() const { return _descriptor->maxValue; }

    void setIntrinsicValue(float v) { _intrinsicValue = v; }
    float intrinsicValue() const { return _intrinsicValue; }
    
    // returns either the intrinsic bus with the current quantum's values,
    // or the overridingInput's output bus.
    // To determine the current value, fetch the bus and make a determination
    // from that.
    std::shared_ptr<AudioBus> bus() const;

    // if the param is constant it is either the intrinsic value, or otherwise
    // held. value will be set to the first stored in the bus by the most recent
    // call to process().
    bool isConstant(float* value = nullptr) const;

    // a short hand convenience to get the value at the beginning of the quantum
    float value() const {
        float r;
        isConstant(&r);
        return r;
    }
    
    // Connect an audio-rate signal to control this parameter.
    // the signal comes from the first channel of the node's output bus
    static void connect(ContextGraphLock & g,
                        std::shared_ptr<AudioParam>, std::shared_ptr<AudioNode>, int channel);
    static void disconnect(ContextGraphLock & g,
                           std::shared_ptr<AudioParam>, std::shared_ptr<AudioNode>);
    static void disconnectAll(ContextGraphLock & g,
                              std::shared_ptr<AudioParam>);
    
    std::shared_ptr<AudioNode> overridingInput() const { return _overridingInput; }

    // truncate any timeline at t seconds in the future, then hold value
    void setValueAtTime(float value, float t);
    
    // replace any timeline with a ramp to value at t seconds in the future,
    // then hold value
    void linearRampToValueAtTime(float value, float t);
    void exponentialRampToValueAtTime(float value, float t);
    
    // replace any timeline at t seconds in the future,
    // and hold at whatever value is then current
    void holdValueAtTime(float t);

    // replace any timeline at t seconds in the future with
    // a curve (a vector of t, v pairs), and hold at the last value
    void setCurve(const std::vector<AudioParamKnot>&, float t);
};

}  // namespace lab

#endif  // AudioParam_h
