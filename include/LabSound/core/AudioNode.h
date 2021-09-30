
// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2020, The LabSound Authors. All rights reserved.

#pragma once
#ifndef AudioNode_h
#define AudioNode_h

#include "LabSound/core/Mixing.h"
#include "LabSound/core/Profiler.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>


namespace lab
{

// clang-format off
enum PanningMode
{
    PANNING_NONE = 0,
    EQUALPOWER   = 1,
    HRTF         = 2,
    _PanningModeCount
};

enum FilterType
{
    FILTER_NONE = 0,
    LOWPASS     = 1,
    HIGHPASS    = 2,
    BANDPASS    = 3,
    LOWSHELF    = 4,
    HIGHSHELF   = 5,
    PEAKING     = 6,
    NOTCH       = 7,
    ALLPASS     = 8,
    _FilterTypeCount
};

enum OscillatorType
{
    OSCILLATOR_NONE  = 0,
    SINE             = 1,
    FAST_SINE        = 2,
    SQUARE           = 3,
    SAWTOOTH         = 4,
    FALLING_SAWTOOTH = 5,
    TRIANGLE         = 6,
    CUSTOM           = 7,
    _OscillatorTypeCount
};

enum class SchedulingState : int
{
    UNSCHEDULED = 0, // Initial playback state. Created, but not yet scheduled
    SCHEDULED,       // Scheduled to play (via noteOn() or noteGrainOn()), but not yet playing
    FADE_IN,         // First epoch, fade in, then play
    PLAYING,         // Generating sound
    STOPPING,        // Transitioning to finished
    RESETTING,       // Node is resetting to initial, unscheduled state
    FINISHING,       // Playing has finished
    FINISHED         // Node has finished
};
const char* schedulingStateName(SchedulingState);
// clang-format on

class  AudioBus;
class  AudioContext;
class  AudioNode;
class  AudioParam;
struct AudioParamDescriptor;
class  AudioSetting;
struct AudioSettingDescriptor;
class  ContextGraphLock;
class  ContextRenderLock;
class  ConcurrentQueue;

struct AudioNodeSummingInput
{
    explicit AudioNodeSummingInput(AudioNodeSummingInput && rh) noexcept
        : sources(std::move(rh.sources))
        , summingBus(rh.summingBus)
    {
        rh.summingBus = nullptr;
    }
    explicit AudioNodeSummingInput() noexcept;
    ~AudioNodeSummingInput();

    struct Source
    {
        std::shared_ptr<AudioNode> node;
        int out;
    };
    // inputs are summing junctions
    std::vector<Source> sources;
    AudioBus * summingBus = nullptr;

    void clear() noexcept;
    void updateSummingBus(int channels, int size);
};

struct AudioNodeNamedOutput
{
    explicit AudioNodeNamedOutput(const std::string name, AudioBus * bus) noexcept
        : name(name)
        , bus(bus)
    {
    }

    AudioNodeNamedOutput(AudioNodeNamedOutput && rh) noexcept
        : name(rh.name)
        , bus(rh.bus)
    {
        // destructor will delete the pointer, so
        // the move operator allows storing Outputs in a container
        // without accidental deletion
        rh.bus = nullptr;
    }

    ~AudioNodeNamedOutput();
    std::string name;
    AudioBus * bus = nullptr;
};


class AudioNodeScheduler
{
public:
    explicit AudioNodeScheduler() = delete;
    explicit AudioNodeScheduler(float sampleRate);
    ~AudioNodeScheduler() = default;

    // Scheduling.
    void start(double when);
    void stop(double when);
    void finish(ContextRenderLock&);  // Called when there is no more sound to play or the noteOff/stop() time has been reached.
    void reset();

    SchedulingState playbackState() const { return _playbackState; }
    bool hasFinished() const { return _playbackState == SchedulingState::FINISHED; }

    bool update(ContextRenderLock&, int epoch_length);

    SchedulingState _playbackState = SchedulingState::UNSCHEDULED;

    bool isCurrentEpoch(ContextRenderLock & r) const;

    // epoch is a long count at sample rate; 136 years at 48kHz
    // For use in an interstellar sound installation or radio frequency signal processing, 
    // please consider upgrading these to uint64_t or write some rollover logic.

    uint64_t _epoch = 0;        // the epoch rendered currently in the busses
    uint64_t _epochLength = 0;  // number of frames in current epoch

    uint64_t _startWhen = std::numeric_limits<uint64_t>::max();    // requested start in epochal coordinate system
    uint64_t _stopWhen = std::numeric_limits<uint64_t>::max();     // requested end in epochal coordinate system

    int _renderOffset = 0; // where rendering starts in the current frame
    int _renderLength = 0; // number of rendered frames in the current frame 

    float _sampleRate = 1;

    std::function<void()> _onEnded;

    std::function<void(double when)> _onStart;
};


struct AudioNodeDescriptor
{
    AudioParamDescriptor * params;
    AudioSettingDescriptor * settings;

    AudioParamDescriptor const * const param(char const * const) const;
    AudioSettingDescriptor const * const setting(char const * const) const;
};

// An AudioNode is the basic building block for handling audio within an AudioContext.
// It may be an audio source, an intermediate processing module, or an audio destination.
// Each AudioNode can have inputs and/or outputs.
// An AudioHardwareDeviceNode has one input and no outputs and represents the final destination to the audio hardware.
// Most processing nodes such as filters will have one input and one output, although multiple inputs and outputs are possible.
class AudioNode
{
    static void _printGraph(const AudioNode * root, std::function<void(const char *)> prnln, int indent);

public :
    enum : int
    {
        ProcessingSizeInFrames = 128
    };

    AudioNode() = delete;
    virtual ~AudioNode();

    explicit AudioNode(AudioContext &, AudioNodeDescriptor const &);

    static void printGraph(const AudioNode* root, std::function<void(const char *)> prnln);

    //--------------------------------------------------
    // required interface
    //
    virtual const char* name() const = 0;

    // The input busses (if any) will already have their input data available when process() is called.
    // Subclasses will take this input data and render into this node's output buses.
    // Called from context's audio thread.
    virtual void process(ContextRenderLock &, int bufferSize) = 0;

    // Resets DSP processing state (clears delay lines, filter memory, etc.)
    // Called from context's audio thread.
    virtual void reset(ContextRenderLock &) = 0;

    // tailTime() is the length of time (not counting latency time) where non-zero output may occur after continuous silent input.
    virtual double tailTime(ContextRenderLock & r) const = 0;

    // latencyTime() is the length of time it takes for non-zero output to appear after non-zero input is provided. This only applies to
    // processing delay which is an artifact of the processing algorithm chosen and is *not* part of the intrinsic desired effect. For
    // example, a "delay" effect is expected to delay the signal, and thus would not be considered latency.
    virtual double latencyTime(ContextRenderLock & r) const = 0;

    //--------------------------------------------------
    // required interface
    //
    // If the final node class has ScheduledNode in its class hierarchy, this will return true.
    // This is to save the cost of a dynamic_cast when scheduling nodes.
    virtual bool isScheduledNode() const { return false; }

    // No significant resources should be allocated until initialize() is called.
    // Processing may not occur until a node is initialized.
    virtual void initialize();
    virtual void uninitialize();
    bool isInitialized() const { return m_isInitialized; }

    void addInput(const std::string & name);
    void addOutput(const std::string & name, int channels, int bufferSize);
    void connect(ContextRenderLock&, int inputIndex, std::shared_ptr<AudioNode> n, int node_outputIndex);
    void connect(int inputIndex, std::shared_ptr<AudioNode> n, int node_outputIndex);
    void disconnect(int inputIndex);
    void disconnect(std::shared_ptr<AudioNode>);
    void disconnect(ContextRenderLock&, std::shared_ptr<AudioNode>);
    void disconnectAll();
    bool isConnected(std::shared_ptr<AudioNode>);

    int numberOfInputs() const { return static_cast<int>(_inputs.size()); }
    int numberOfOutputs() const { return static_cast<int>(_outputs.size()); }

    ///  @TODO make these all const
    AudioBus * inputBus(ContextRenderLock & r, int i);
    AudioBus * outputBus(ContextRenderLock & r, int i);
    AudioBus * outputBus(ContextRenderLock& r, char const* const str);
    std::string outputBusName(ContextRenderLock& r, int i);

    // processIfNecessary() is called by our output(s) when the rendering graph needs this AudioNode to process.
    // This method ensures that the AudioNode will only process once per rendering time quantum even if it's called repeatedly.
    // This handles the case of "fanout" where an output is connected to multiple AudioNode inputs.
    // Called from context's audio thread.
    void processIfNecessary(ContextRenderLock & r, int bufferSize);

    // propagatesSilence() should return true if the node will generate silent output when given silent input. By default, AudioNode
    // will take tailTime() and latencyTime() into account when determining whether the node will propagate silence.
    virtual bool propagatesSilence(ContextRenderLock & r) const;

    bool inputsAreSilent(ContextRenderLock &);
    void silenceOutputs(ContextRenderLock &);
    void unsilenceOutputs(ContextRenderLock &);

    int channelCount();
    void setChannelCount(ContextGraphLock & g, int channelCount);

    ChannelCountMode channelCountMode() const { return m_channelCountMode; }
    void setChannelCountMode(ContextGraphLock & g, ChannelCountMode mode);

    ChannelInterpretation channelInterpretation() const { return m_channelInterpretation; }
    void setChannelInterpretation(ChannelInterpretation interpretation) { m_channelInterpretation = interpretation; }

    // returns a vector of parameter names
    std::vector<std::string> paramNames() const;
    std::vector<std::string> paramShortNames() const;

    // returns a vector of setting names
    std::vector<std::string> settingNames() const;
    std::vector<std::string> settingShortNames() const;

    std::shared_ptr<AudioParam> param(char const * const str);
    std::shared_ptr<AudioSetting> setting(char const * const str);

    std::vector<std::shared_ptr<AudioParam>> params() const { return _params; }
    std::vector<std::shared_ptr<AudioSetting>> settings() const { return _settings; }

    AudioNodeScheduler _scheduler;

    ProfileSample graphTime;    // how much time the node spend pulling inputs
    ProfileSample totalTime;    // total time spent by the node. total-graph is the self time.

    // Called by processIfNecessary() to recurse the audiograph.
    // recursion stops if a node's quantum is equal to the context's render quantum
    void pullInputs(ContextRenderLock &, int bufferSize);

protected:

    friend class AudioContext;

    bool m_isInitialized {false};

    struct Work
    {
        enum Op
        {
            OpConnectInput,
            OpDisconnectIndex, 
            OpDisconnectInput,
            OpAddInput,
            OpAddOutput
        };

        Op op;

        // if it's an input
        std::shared_ptr<AudioNode> node;
        int node_outputIndex = 0;
        int inputIndex = 0;

        // if it's an output
        std::string name;
        int channelCount = 0, size = 0;
    };

    ConcurrentQueue * _enqueudWork;

    void serviceQueue(ContextRenderLock & r);

    std::vector<AudioNodeSummingInput> _inputs;
    std::vector<AudioNodeNamedOutput> _outputs;

    std::vector<std::shared_ptr<AudioParam>> _params;
    std::vector<std::shared_ptr<AudioSetting>> _settings;

    int m_channelCount{ 0 };

    ChannelCountMode m_channelCountMode{ ChannelCountMode::Max };
    ChannelInterpretation m_channelInterpretation{ ChannelInterpretation::Speakers };

    // starts an immediate ramp to zero in preparation for disconnection
    void scheduleDisconnect() { _scheduler.stop(0); }

    // returns true if the disconnection ramp has reached zero.
    // This is intended to allow the AudioContext to manage popping artifacts
    bool disconnectionReady() const { return _scheduler._playbackState != SchedulingState::PLAYING; }
};

}  // namespace lab

#endif  // AudioNode_h

