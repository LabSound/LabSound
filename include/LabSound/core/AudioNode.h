
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

    
// tiny stand-in for std::vector that keeps only an array of pointers
// and uses an exponential growth policy, as this is most suitable for
// inputs and outputs, because 1, 2, 4, and 8, are all likely explicitly
// requested numbers of inputs and outputs. 
//
template<typename T>
struct Vector {

    Vector() = default;

    Vector(Vector&& rh) noexcept {
        data = rh.data;
        cap = rh.cap;
        sz = rh.sz;
        rh.data = nullptr;
        rh.cap = 0;
        rh.sz = 0;
    }
    ~Vector() {
        clear();
    }
    T** data = nullptr;
    size_t cap = 0;
    size_t sz = 0;

    void clear() {
        for (int i = 0; i < sz; ++i) {
            if (data[i]) 
                delete data[i];
            data[i] = nullptr;
        }
        sz = 0;
    }

    void emplace_back(T && rh) {
        if (!cap) {
            data = (T**) malloc(sizeof(T*));
            sz = 1;
            cap = 1;
            *data = new T(std::move(rh));
        }
        else {
            if (sz < cap) {
                data[sz] = new T(std::move(rh));
                ++sz;
            }
            else {
                resize(sz + 1);
                data[sz] = new T(std::move(rh));
                ++sz;
            }
        }
    }

    void emplace_back(T * rh)
    {
        if (!cap)
        {
            data = (T **) malloc(sizeof(T *));
            sz = 1;
            cap = 1;
            *data = rh;
        }
        else
        {
            if (sz < cap)
            {
                data[sz] = rh;
                ++sz;
            }
            else
            {
                resize(sz + 1);
                data[sz] = rh;
                ++sz;
            }
        }
    }

    void swap_pop(int j) {
        if (!sz)
            return;

        if (j < sz - 1) {
            data[j] = data[sz - 1];
            data[sz - 1] = 0;
        }
        --sz;
    }

    void pop_back() {
        if (sz > 0)
        {
            --sz;
            if (data[sz])
                delete data[sz];
            data[sz] = nullptr;
        }
    }

    T ** begin() const
    {
        return data;
    }
    T ** end() const
    {
        return data + sz;
    }

    void resize(size_t i) {
        if (i >= cap) {
            size_t new_cap = i + 1;
            T ** new_data = (T **) malloc(sizeof(T *) * new_cap);
            memset(new_data, 0, sizeof(T *) * cap);
            memcpy(new_data, data, sizeof(T *) * sz);  // copy pointers
            free(data);
            data = new_data;
            cap = new_cap;
        }
    }

    size_t size() const { return sz; }

    T* operator[](size_t i) const {
        if (i >= sz)
            return nullptr;

        return data[i];
    }

    T*& operator[](size_t i) {
        resize(i);
        return data[i];
    }
};


// clang-format off
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
class  ContextRenderLock;
class  ConcurrentQueue;

struct AudioNodeSummingInput
{
    explicit AudioNodeSummingInput() noexcept = default;
    explicit AudioNodeSummingInput(const std::string name) noexcept;
    explicit AudioNodeSummingInput(AudioNodeSummingInput && rh) noexcept
        : sources(std::move(rh.sources))
        , summingBus(rh.summingBus)
    {
        std::swap(name, rh.name);
        rh.summingBus = nullptr;
    }
    ~AudioNodeSummingInput();

    // inputs are summing junctions
    Vector<std::shared_ptr<AudioNode>> sources;
    AudioBus * summingBus = nullptr;
    std::string name;

    void clear() noexcept;
};

class AudioNodeScheduler
{
public:
    explicit AudioNodeScheduler() = delete;
    explicit AudioNodeScheduler(float sampleRate);
    ~AudioNodeScheduler() = default;

    // Scheduling
    void start(double when);
    void stop(double when);

    // called when the sound is finished, or noteOff/stop time has been reached
    void finish(ContextRenderLock&);
    void reset();

    SchedulingState playbackState() const { return _playbackState; }
    bool hasFinished() const { return _playbackState == SchedulingState::FINISHED; }

    bool update(ContextRenderLock&, int epoch_length);

    SchedulingState _playbackState = SchedulingState::UNSCHEDULED;

    bool isCurrentEpoch(ContextRenderLock & r) const;

    // epoch is a long count at sample rate; 136 years at 48kHz

    uint64_t _epoch = 0;        // the epoch rendered currently in the busses
    uint64_t _epochLength = 0;  // number of frames in current epoch

    // start and stop epoch (absolute, not relative)
    uint64_t _startWhen = std::numeric_limits<uint64_t>::max();
    uint64_t _stopWhen = std::numeric_limits<uint64_t>::max();

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

// xAudioNode is the basic building block for a signal processing graph.
// It may be an audio source, an intermediate processing module, or an audio 
// destination.
//
// Each AudioNode can have inputs and/or outputs.
// An AudioHardwareDeviceNode has one input and no outputs and represents the 
// final destination to the audio hardware.
// Most processing nodes such as filters will have one input and one output, 
//
class AudioNode
{
    static void _printGraph(const AudioNode * root, 
                        std::function<void(const char *)> prnln, int indent);

public :
    enum : int
    {
        ProcessingSizeInFrames = 128
    };

    int color = 0;

    AudioNode() = delete;
    virtual ~AudioNode();

    explicit AudioNode(AudioContext &, AudioNodeDescriptor const &);

    static void printGraph(const AudioNode* root, 
                        std::function<void(const char *)> prnln);

    //--------------------------------------------------
    // required interface
    //
    virtual const char* name() const = 0;

    // The input busses (if any) will already have their input data available
    // when process() is called. Subclasses will take this input data and 
    // render into this node's output buses.
    // Called from context's audio thread.
    virtual void process(ContextRenderLock &, int bufferSize) = 0;

    // Resets DSP processing state (clears delay lines, filter memory, etc.)
    // Called from context's audio thread.
    virtual void reset(ContextRenderLock &) = 0;

    // tailTime() is the length of time (not counting latency time) where 
    // non-zero output may occur after continuous silent input.
    virtual double tailTime(ContextRenderLock & r) const = 0;

    // latencyTime() is the length of time it takes for non-zero output to 
    // appear after non-zero input is provided. This only applies to processing
    // delay which is an artifact of the processing algorithm chosen and is 
    // *not* part of the intrinsic desired effect. For example, a "delay" 
    // effect is expected to delay the signal, and thus would not be considered
    // latency.
    virtual double latencyTime(ContextRenderLock & r) const = 0;

    //--------------------------------------------------
    // overridable interface
    //
    // If the final node class has ScheduledNode in its class hierarchy, this 
    // will return true. This is to save the cost of a dynamic_cast when
    // scheduling nodes.
    virtual bool isScheduledNode() const { return false; }

    // propagatesSilence() should return true if the node will generate silent
    // output when given silent input. By default, AudioNode
    // will take tailTime() and latencyTime() into account when determining
    // whether the node will propagate silence.
    virtual bool propagatesSilence(ContextRenderLock & r) const;

    // No significant resources should be allocated until initialize() is called.
    // Processing may not occur until a node is initialized.
    virtual void initialize();
    virtual void uninitialize();

    //--------------------------------------------------
    // standard interface

    // inputs and outputs
    bool isInitialized() const { return m_isInitialized; }

    void addInput(const std::string & name);

    const Vector<AudioNodeSummingInput>& inputs() const { return _inputs; }
    const AudioNodeSummingInput* input(int index) const;
    const AudioBus* output() const;
    int input_index(const char* name) const;

    int numberOfInputs() const { return static_cast<int>(_inputs.size()); }

    // connections 
    void connect(ContextRenderLock&, 
            int inputIndex, std::shared_ptr<AudioNode> n);
    void connect(int inputIndex, std::shared_ptr<AudioNode> n);
    void disconnect(int inputIndex);
    void disconnect(std::shared_ptr<AudioNode>);
    void disconnect(ContextRenderLock&, std::shared_ptr<AudioNode>);
    void disconnectAll();
    bool isConnected(std::shared_ptr<AudioNode>);

    // audio busses
    ///  @TODO make these all const
    AudioBus * inputBus(ContextRenderLock & r, int i);
    AudioBus * outputBus(ContextRenderLock& r);
    std::string inputBusName(ContextRenderLock & r, int i);

    // parameters
    std::vector<std::string> paramNames() const;
    std::vector<std::string> paramShortNames() const;
    std::shared_ptr<AudioParam> param(char const * const str);
    std::shared_ptr<AudioParam> param(int index);
    int param_index(char const * const str);
    std::vector<std::shared_ptr<AudioParam>> params() const { return _params; }

    //  settings
    std::vector<std::string> settingNames() const;
    std::vector<std::string> settingShortNames() const;
    std::shared_ptr<AudioSetting> setting(char const * const str);
    std::shared_ptr<AudioSetting> setting(int index);
    int setting_index(char const * const str);
    std::vector<std::shared_ptr<AudioSetting>> settings() const { return _settings; }

    // miscelleaneous management
    AudioNodeScheduler _scheduler;

    // channel interpretation
    ChannelInterpretation channelInterpretation() const { 
        return m_channelInterpretation; }
    void setChannelInterpretation(ChannelInterpretation interpretation) {
        m_channelInterpretation = interpretation; }

    ProfileSample graphTime;    // how much time the node spent pulling inputs
    ProfileSample totalTime;    // total time spent by the node. 
                                // total - graph is the self time.

    // Recurse the audiograph, filling in the the node's output audio bus with
    // bufferSize samples.
    // recursion stops if a node's quantum is equal to the context's render quantum,
    // which happens if a node's output is connected to more than one input.
    // Called from a device's audio callback, offline processors, and so on.
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

    int _channelCount = 1;

    Vector<AudioNodeSummingInput> _inputs;
    AudioBus* _output;

    std::vector<std::shared_ptr<AudioParam>> _params;
    std::vector<std::shared_ptr<AudioSetting>> _settings;

    ChannelInterpretation m_channelInterpretation{ ChannelInterpretation::Speakers };
    // 
    bool inputsAreSilent(ContextRenderLock &);
    void silenceOutputs(ContextRenderLock &);
    void unsilenceOutputs(ContextRenderLock &);

    // starts an immediate ramp to zero in preparation for disconnection
    void scheduleDisconnect() { _scheduler.stop(0); }

    // returns true if the disconnection ramp has reached zero.
    // This is intended to allow the AudioContext to manage popping artifacts
    bool disconnectionReady() const { return _scheduler._playbackState != SchedulingState::PLAYING; }
};

}  // namespace lab

#endif  // AudioNode_h

