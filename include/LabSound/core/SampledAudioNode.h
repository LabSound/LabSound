
// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef SampledAudioNode_h
#define SampledAudioNode_h

#include "LabSound/core/AudioScheduledSourceNode.h"

#include <memory>


namespace lab
{

class AudioContext;
class AudioBus;
class AudioParam;
class ContextRenderLock;
class SampledAudioNode;
struct SRC_Resampler;

// SampledAudioNode is intended for in-memory sounds. It provides a high degree of scheduling 
// flexibility (can playback in rhythmically exact ways).


class SampledAudioNode final : public AudioScheduledSourceNode
{
    virtual void reset(ContextRenderLock& r) override {}
    virtual double tailTime(ContextRenderLock& r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock& r) const override { return 0; }
    virtual bool propagatesSilence(ContextRenderLock& r) const override { return false; }

    struct Scheduled;
    struct Internals;
    Internals* _internals;

    std::shared_ptr<AudioSetting> m_sourceBus;
    std::shared_ptr<AudioBus> m_pendingSourceBus;   // the most recently assigned bus
    std::shared_ptr<AudioBus> m_retainedSourceBus;  // the bus used in computation, eventually agrees with m_pendingSourceBus.
    std::shared_ptr<AudioParam> m_playbackRate;
    std::shared_ptr<AudioParam> m_detune;
    std::shared_ptr<AudioParam> m_dopplerRate;

    std::vector<std::shared_ptr<SRC_Resampler>> _resamplers;

    // totalPitchRate() returns the instantaneous pitch rate (non-time preserving).
    // It incorporates the base pitch rate, any sample-rate conversion factor from the buffer, 
    // and any doppler shift from an associated panner node.
    float totalPitchRate(ContextRenderLock&);
    bool renderSample(ContextRenderLock& r, Scheduled&, size_t destinationSampleOffset, size_t frameSize);

    virtual void process(ContextRenderLock&, int framesToProcess) override;
    static bool s_registered;

public:
    SampledAudioNode() = delete;
    explicit SampledAudioNode(AudioContext&);
    virtual ~SampledAudioNode();

    static const char* static_name() { return "SampledAudio"; }
    virtual const char* name() const override { return static_name(); }

    // setting the bus is an asynchronous operation. getBus returns the most
    // recent set request in order that the interface work in a predictable way.
    // In the future, setBus and getBus could be deprecated in favor of another
    // schedule method that takes a source bus as an argument.
    void setBus(ContextRenderLock&, std::shared_ptr<AudioBus> sourceBus);
    std::shared_ptr<AudioBus> getBus() const { return m_pendingSourceBus; }

    // loopCount of -1 will loop forever
    // all the schedule routines will call start(0) if necessary, so that a
    // schedule is sufficient for this node to start producing a signal.
    void schedule(double when);
    void schedule(double when, int loopCount);
    void schedule(double when, double grainOffset, int loopCount);
    void schedule(double when, double grainOffset, double grainDuration, int loopCount);

    // this will clear anything playing or pending, without stopping the node itself.
    void clearSchedules();

    std::shared_ptr<AudioParam> playbackRate() { return m_playbackRate; }
    std::shared_ptr<AudioParam> detune() { return m_detune; }
    std::shared_ptr<AudioParam> dopplerRate() { return m_dopplerRate; }
};



}  // namespace lab

#endif  // SampledAudioNode
