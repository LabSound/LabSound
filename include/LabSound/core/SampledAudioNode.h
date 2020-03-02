// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef SampledAudioNode_h
#define SampledAudioNode_h

#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/PannerNode.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/extended/AudioContextLock.h"

#include <memory>
#include <list>

namespace lab
{

class AudioContext;
class AudioBus;

// This should  be used for short sounds which require a high degree of scheduling flexibility (can playback in rhythmically perfect ways).

///////////////////////////
//   SampledAudioVoice   //
///////////////////////////

class SampledAudioVoice : public AudioScheduledSourceNode
{
    std::shared_ptr<AudioParam> m_gain;
    std::shared_ptr<AudioParam> m_playbackRate;
    std::shared_ptr<AudioParam> m_detune;
    std::shared_ptr<AudioSetting> m_isLooping;
    std::shared_ptr<AudioSetting> m_loopStart;
    std::shared_ptr<AudioSetting> m_loopEnd;
    std::shared_ptr<AudioSetting> m_sourceBus;

    // Transient
    double m_virtualReadIndex;
    bool m_isGrain{false};
    double m_grainOffset{0};  // in seconds
    double m_grainDuration{0.025};  // in 25 ms                      
    float m_lastGain{1.0f}; // m_lastGain provides continuity when we dynamically adjust the gain.
    float m_totalPitchRate;

    // Scheduling
    bool m_startRequested{false};
    double m_requestWhen{0};
    double m_requestGrainOffset{0};
    double m_requestGrainDuration{0};

    // Setup
    float m_duration;

    // Although we inhert from AudioScheduledSourceNode, we does not directly connect to any outputs
    // since that is handled by the parent SampledAudioNode. 
    std::shared_ptr<AudioNodeOutput> m_output;

public:

    bool m_channelSetupRequested{false};
    std::shared_ptr<AudioBus> m_inPlaceBus;

    SampledAudioVoice(float grain_dur, std::shared_ptr<AudioParam> gain, std::shared_ptr<AudioParam> rate, 
        std::shared_ptr<AudioParam> detune, std::shared_ptr<AudioSetting> loop, std::shared_ptr<AudioSetting> loop_s, 
        std::shared_ptr<AudioSetting> loop_e,  std::shared_ptr<AudioSetting> src_bus);

    virtual ~SampledAudioVoice() {if (isInitialized()) uninitialize(); };

    void setOutput(ContextRenderLock & r, std::shared_ptr<AudioNodeOutput> parent_sampled_audio_node_output)
    {
        auto parentOutputBus = parent_sampled_audio_node_output->bus(r);
        m_inPlaceBus = std::make_shared<AudioBus>(parentOutputBus->numberOfChannels(), parentOutputBus->length());
        m_output = parent_sampled_audio_node_output;
    }

    size_t numberOfChannels(ContextRenderLock & r);

    void schedule(double when, double grainOffset);
    void schedule(double when, double grainOffset, double grainDuration);

    // Returns true if we're finished.
    bool renderSilenceAndFinishIfNotLooping(ContextRenderLock & r, AudioBus * bus, size_t index, size_t framesToProcess);
    bool renderSample(ContextRenderLock & r, AudioBus * bus, size_t destinationSampleOffset, size_t frameSize);
    void setPitchRate(ContextRenderLock & r, const float rate) { m_totalPitchRate = rate; }

    virtual void process(ContextRenderLock & r, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock & r) override;
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
};

//////////////////////////
//   SampledAudioNode   //
//////////////////////////

class SampledAudioNode final : public AudioNode
{
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
    virtual bool propagatesSilence(ContextRenderLock & r) const override;

    std::shared_ptr<AudioSetting> m_sourceBus;
    std::unique_ptr<AudioBus> m_summingBus;

    // If m_isLooping is false, then this node will be done playing and become inactive after it reaches the end of the sample data in the buffer.
    // If true, it will wrap around to the start of the buffer each time it reaches the end.
    std::shared_ptr<AudioParam> m_gain;
    std::shared_ptr<AudioParam> m_playbackRate;
    std::shared_ptr<AudioParam> m_detune;
    std::shared_ptr<AudioSetting> m_isLooping;
    std::shared_ptr<AudioSetting> m_loopStart;
    std::shared_ptr<AudioSetting> m_loopEnd;

    // totalPitchRate() returns the instantaneous pitch rate (non-time preserving).
    // It incorporates the base pitch rate, any sample-rate conversion factor from the buffer, and any doppler shift from an associated panner node.
    double totalPitchRate(ContextRenderLock &);

    // We optionally keep track of a panner node which has a doppler shift that is incorporated into
    // the pitch rate. We manually manage ref-counting because we want to use RefTypeConnection.
    PannerNode * m_pannerNode{nullptr};

    struct ScheduleRequest
    {
        double when;
        double grainOffset;
        double grainDuration;
        ScheduleRequest(double when, double offset, double duration) : when(when), grainOffset(offset), grainDuration(duration) {}
    };
    friend bool operator < (const ScheduleRequest & a, const ScheduleRequest & b) { return a.when < b.when; }
    friend bool operator == (const ScheduleRequest & a, const ScheduleRequest & b) { return a.when == b.when; }

    std::vector<std::unique_ptr<SampledAudioVoice>> voices;
    std::list<ScheduleRequest> schedule_list;
    std::function<void()> m_onEnded;

public:

    static constexpr size_t MAX_NUM_VOICES = 32;

    SampledAudioNode();
    virtual ~SampledAudioNode();

    virtual void process(ContextRenderLock &, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock &) override;

    bool setBus(ContextRenderLock &, std::shared_ptr<AudioBus> sourceBus);
    std::shared_ptr<AudioBus> getBus() const { return m_sourceBus->valueBus(); }

    // Play-state
    float duration() const;
    void schedule(double when, double grainOffset = 0.0);
    void schedule(double when, double grainOffset, double grainDuration);

    std::shared_ptr<AudioSetting> isLooping() { return m_isLooping; } 
    std::shared_ptr<AudioSetting> loopStart() { return m_loopStart; } 
    std::shared_ptr<AudioSetting> loopEnd() { return m_loopEnd; } 

    std::shared_ptr<AudioParam> gain() { return m_gain; }
    std::shared_ptr<AudioParam> playbackRate() { return m_playbackRate; }
    std::shared_ptr<AudioParam> detune() { return m_detune; }

    void setOnEnded(std::function<void()> fn) { m_onEnded = fn; }

    // If a panner node is set, then we can incorporate doppler shift into the playback pitch rate.
    void setPannerNode(PannerNode *);
    virtual void clearPannerNode() override;
};

}  // namespace lab

#endif  // SampledAudioNode
