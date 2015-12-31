// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ConvolverNode_h
#define ConvolverNode_h

#include "LabSound/core/AudioNode.h"

namespace lab {

class AudioBuffer;
class Reverb;
    
class ConvolverNode : public AudioNode {
public:
    ConvolverNode(float sampleRate);
    virtual ~ConvolverNode();
    
    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;
    virtual void initialize() override;
    virtual void uninitialize() override;

    // Impulse responses
    void setBuffer(ContextGraphLock&, std::shared_ptr<AudioBuffer>);
    std::shared_ptr<AudioBuffer> buffer();

    bool normalize() const { return m_normalize; }
    void setNormalize(bool normalize) { m_normalize = normalize; }

private:
    virtual double tailTime() const override;
    virtual double latencyTime() const override;

    std::unique_ptr<Reverb> m_reverb;
    std::shared_ptr<AudioBuffer> m_buffer;

    // lock free swap on update
    bool m_swapOnRender;
    std::unique_ptr<Reverb> m_newReverb;
    std::shared_ptr<AudioBuffer> m_newBuffer;

    // Normalize the impulse response or not. Must default to true.
    bool m_normalize;
};

} // namespace lab

#endif // ConvolverNode_h
