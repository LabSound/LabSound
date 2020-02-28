// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ConvolverNode_h
#define ConvolverNode_h

#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioSetting.h"

#include <memory>

namespace lab
{

class AudioBus;
class AudioSetting;
class Reverb;

namespace deprecated
{
    // Copyright (C) 2010, Google Inc. All rights reserved.

    // params:
    // settings: normalize
    //
    class ConvolverNode final : public AudioNode
    {
    public:
        ConvolverNode();
        virtual ~ConvolverNode();

        // AudioNode
        virtual void process(ContextRenderLock &, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock &) override;
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

}  // deprecated

// private data for reverb computations
struct sp_data;
struct sp_conv;
struct sp_ftbl;

class ConvolverNode final : public AudioScheduledSourceNode
{
public:
    ConvolverNode();
    virtual ~ConvolverNode();
    bool normalize() const;
    void setNormalize(bool new_n);

    // set impulse will schedule the convolver to begin processing immediately
    // The supplied bus is copied for use as an impulse response.
    void setImpulse(std::shared_ptr<AudioBus> bus);
    std::shared_ptr<AudioBus> getImpulse() const;
    virtual void process(ContextRenderLock & r, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock &) override;

protected:
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
    virtual bool propagatesSilence(ContextRenderLock & r) const override;
    double now() const { return _now; }

    void _activateNewImpulse();

    double _now = 0.0;
    float _scale = 1.f;  // normalization value
    sp_data * _sp = nullptr;

    // Normalize the impulse response or not. Must default to true.
    std::shared_ptr<AudioSetting> _normalize;
    std::shared_ptr<AudioSetting> _impulseResponseClip;

    struct ReverbKernel
    {
        ReverbKernel() = default;
        ReverbKernel(ReverbKernel && rh) noexcept;
        ~ReverbKernel();
        sp_conv * conv = nullptr;
        sp_ftbl * ft = nullptr;
    };
    std::vector<ReverbKernel> _kernels;  // one per impulse response channel
};

}  // namespace lab

#endif  // ConvolverNode_h
