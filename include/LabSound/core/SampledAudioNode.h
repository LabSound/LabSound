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
class SampledAudioNode;

// This should  be used for short sounds which require a high degree of scheduling flexibility (can playback in rhythmically perfect ways).

///////////////////////////
//   SampledAudioVoice   //
///////////////////////////

class SampledAudioVoice : public AudioNode
{
    friend class SampledAudioNode;

    std::shared_ptr<AudioParam> m_gain;
    std::shared_ptr<AudioParam> m_playbackRate;
    std::shared_ptr<AudioParam> m_detune;
    std::shared_ptr<AudioSetting> m_isLooping;
    std::shared_ptr<AudioSetting> m_loopStart;
    std::shared_ptr<AudioSetting> m_loopEnd;
    std::shared_ptr<AudioSetting> m_sourceBus;

    // Transient
    double m_virtualReadIndex{ 0 };
    bool m_isGrain{ false };
    double m_grainOffset{ 0 };  // in seconds
    double m_grainDuration{ 0.025 };  // in 25 ms
    float m_lastGain{ 1.0f }; // m_lastGain provides continuity when we dynamically adjust the gain.
    float m_totalPitchRate{ 1.f };

    // Scheduling
    bool m_startRequested{ false };
    double m_requestWhen{ 0 };
    double m_requestGrainOffset{ 0 };
    double m_requestGrainDuration{ 0 };

    // Setup
    float m_duration;

    // Although we inhert from AudioScheduledSourceNode, we does not directly connect to any outputs
    // since that is handled by the parent SampledAudioNode. 
    std::shared_ptr<AudioNodeOutput> m_output;

    // scheduling interface

    static constexpr double UNKNOWN_TIME = -1.0;
    std::function<void()> m_onEnded;

    // These are the possible states an AudioScheduledSourceNode can be in:
    // UNSCHEDULED_STATE - Initial playback state. Created, but not yet scheduled.
    // SCHEDULED_STATE - Scheduled to play, but not yet playing.
    // PLAYING_STATE - Generating sound.
    enum PlaybackState
    {
        UNSCHEDULED_STATE = 0,
        SCHEDULED_STATE = 1,
        PLAYING_STATE = 2,
        FINISHED_STATE = 3
    };


    PlaybackState playbackState() const { return m_playbackState; }
    bool isPlayingOrScheduled() const { return m_playbackState == PLAYING_STATE || m_playbackState == SCHEDULED_STATE; }

    void setOnEnded(std::function<void()> fn) { m_onEnded = fn; }

    // Get frame information for the current time quantum.
    // We handle the transition into PLAYING_STATE and FINISHED_STATE here,
    // zeroing out portions of the outputBus which are outside the range of startFrame and endFrame.
    //
    // Each frame time is relative to the context's currentSampleFrame().
    // quantumFrameOffset    : Offset frame in this time quantum to start rendering.
    // nonSilentFramesToProcess : Number of frames rendering non-silence (will be <= quantumFrameSize).
    void updateSchedulingInfo(ContextRenderLock&,
        size_t quantumFrameSize, AudioBus* outputBus,
        size_t& quantumFrameOffset, size_t& nonSilentFramesToProcess);

    // Called when there is no more sound to play or the noteOff/stop() time has been reached.
    void finish(ContextRenderLock&);

    PlaybackState m_playbackState{ UNSCHEDULED_STATE };

    // m_startTime is the time to start playing based on the context's timeline.
    // 0 or a time less than the context's current time means as soon as the
    // next audio buffer is processed.
    //
    double m_pendingStartTime{ UNKNOWN_TIME };
    double m_startTime{ 0.0 };  // in seconds

    // m_endTime is the time to stop playing based on the context's timeline.
    // 0 or a time less than the context's current time means as soon as the
    // next audio buffer is processed.
    //
    // If it hasn't been set explicitly,
    //    if looping, then the sound will not stop playing
    // else
    //    it will stop when the end of the AudioBuffer has been reached.
    //
    double m_pendingEndTime{ UNKNOWN_TIME };
    double m_endTime{ UNKNOWN_TIME };  // in seconds

    bool m_inSchedule{ false };

// scheduling interface


public:

    bool m_channelSetupRequested{false};
    std::shared_ptr<AudioBus> m_inPlaceBus;

    SampledAudioVoice(AudioContext &, float grain_dur, std::shared_ptr<AudioParam> gain, std::shared_ptr<AudioParam> rate, 
        std::shared_ptr<AudioParam> detune, std::shared_ptr<AudioSetting> loop, std::shared_ptr<AudioSetting> loop_s, 
        std::shared_ptr<AudioSetting> loop_e,  std::shared_ptr<AudioSetting> src_bus);

    virtual ~SampledAudioVoice() {if (isInitialized()) uninitialize(); };

    static const char* static_name() { return "SampledAudioVoice"; }
    virtual const char* name() const override { return static_name(); }

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

    virtual void process(ContextRenderLock & r, int framesToProcess) override;
    virtual void reset(ContextRenderLock & r) override;
    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
};

//////////////////////////
//   SampledAudioNode   //
//////////////////////////

class SampledAudioNode final : public AudioScheduledSourceNode
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

    void _createVoicesForNewBus(ContextRenderLock & r);
    bool m_resetVoices{ false };
    bool m_inSchedule{ false };

public:

    static constexpr size_t MAX_NUM_VOICES = 32;

    SampledAudioNode(AudioContext &);
    virtual ~SampledAudioNode();

    static const char* static_name() { return "SampledAudio"; }
    virtual const char* name() const override { return static_name(); }

    virtual void process(ContextRenderLock &, int framesToProcess) override;
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
