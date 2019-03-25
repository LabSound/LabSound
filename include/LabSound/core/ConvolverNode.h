// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ConvolverNode_h
#define ConvolverNode_h

#include "LabSound/core/AudioNode.h"

#include <memory>

namespace lab {

class AudioBus;
class AudioSetting;
class Reverb;

// params:
// settings: normalize
//    
class ConvolverNode final : public AudioNode 
{
public:

    ConvolverNode();
    virtual ~ConvolverNode();
    
    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;
    virtual void initialize() override;
    virtual void uninitialize() override;

    // Impulse responses
    // setImpulse takes an audio bus as a source of a buffer to create an audio
    // bus from, but the bus and its data is not retained
    void setImpulse(std::shared_ptr<AudioBus> bus);
    std::shared_ptr<AudioBus> getImpulse();

    bool normalize() const;
    void setNormalize(bool normalize);

private:

    virtual double tailTime(ContextRenderLock & r) const override;
    virtual double latencyTime(ContextRenderLock & r) const override;

    std::unique_ptr<Reverb> m_reverb;
    std::shared_ptr<AudioBus> m_bus;

    // lock free swap on update
    bool m_swapOnRender;
    std::unique_ptr<Reverb> m_newReverb;
    std::shared_ptr<AudioBus> m_newBus;

    // Normalize the impulse response or not. Must default to true.
    std::shared_ptr<AudioSetting> m_normalize;
};

} // namespace lab

#endif // ConvolverNode_h
