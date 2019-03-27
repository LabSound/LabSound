// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

// @tofix - webkit change 1f083e8 and 2bd2dc2 adds support for different behaviors on mixing such as
// clamping the max number of channels, and mixing 5.1 down to mono

#ifndef AudioNode_h
#define AudioNode_h

#include "LabSound/core/Mixing.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>
#include <iostream>

namespace lab
{

enum PanningMode
{
    PANNING_NONE = 0,
    EQUALPOWER = 10,
    HRTF = 20,
};

enum FilterType
{
    FILTER_NONE = 0,
    LOWPASS = 1,
    HIGHPASS = 2,
    BANDPASS = 3,
    LOWSHELF = 4,
    HIGHSHELF = 5,
    PEAKING = 6,
    NOTCH = 7,
    ALLPASS = 8
};

enum class OscillatorType
{
    OSCILLATOR_NONE = 0,
    SINE = 1,
    SQUARE = 2,
    SAWTOOTH = 3,
    TRIANGLE = 4,
    CUSTOM = 5
};

class AudioContext;
class AudioNodeInput;
class AudioNodeOutput;
class AudioParam;
class AudioSetting;
class ContextGraphLock;
class ContextRenderLock;

// An AudioNode is the basic building block for handling audio within an AudioContext.
// It may be an audio source, an intermediate processing module, or an audio destination.
// Each AudioNode can have inputs and/or outputs. An AudioSourceNode has no inputs and a single output.
// An AudioDestinationNode has one input and no outputs and represents the final destination to the audio hardware.
// Most processing nodes such as filters will have one input and one output, although multiple inputs and outputs are possible.
class AudioNode
{
public:

    enum
    {
        ProcessingSizeInFrames = 128
    };

    AudioNode();
    virtual ~AudioNode();

    // LabSound: If the node included ScheduledNode in its hierarchy, this will return true.
    // This is to save the cost of a dynamic_cast when scheduling nodes.
    virtual bool isScheduledNode() const { return false; }

    // The AudioNodeInput(s) (if any) will already have their input data available when process() is called.
    // Subclasses will take this input data and put the results in the AudioBus(s) of its AudioNodeOutput(s) (if any).
    // Called from context's audio thread.
    virtual void process(ContextRenderLock&, size_t framesToProcess) = 0;

    // Resets DSP processing state (clears delay lines, filter memory, etc.)
    // Called from context's audio thread.
    virtual void reset(ContextRenderLock&) = 0;

    // No significant resources should be allocated until initialize() is called.
    // Processing may not occur until a node is initialized.
    virtual void initialize();
    virtual void uninitialize();

    bool isInitialized() const { return m_isInitialized; }

    size_t numberOfInputs() const { return m_inputs.size(); }
    size_t numberOfOutputs() const { return m_outputs.size(); }

    std::shared_ptr<AudioNodeInput> input(size_t index);
    std::shared_ptr<AudioNodeOutput> output(size_t index);

    // processIfNecessary() is called by our output(s) when the rendering graph needs this AudioNode to process.
    // This method ensures that the AudioNode will only process once per rendering time quantum even if it's called repeatedly.
    // This handles the case of "fanout" where an output is connected to multiple AudioNode inputs.
    // Called from context's audio thread.
    void processIfNecessary(ContextRenderLock& r, size_t framesToProcess);

    // Called when a new connection has been made to one of our inputs or the connection number of channels has changed.
    // This potentially gives us enough information to perform a lazy initialization or, if necessary, a re-initialization.
    // Called from main thread.
    virtual void checkNumberOfChannelsForInput(ContextRenderLock&, AudioNodeInput*);

    // tailTime() is the length of time (not counting latency time) where non-zero output may occur after continuous silent input.
    virtual double tailTime(ContextRenderLock & r) const = 0;

    // latencyTime() is the length of time it takes for non-zero output to appear after non-zero input is provided. This only applies to
    // processing delay which is an artifact of the processing algorithm chosen and is *not* part of the intrinsic desired effect. For
    // example, a "delay" effect is expected to delay the signal, and thus would not be considered latency.
    virtual double latencyTime(ContextRenderLock & r) const = 0;

    // propagatesSilence() should return true if the node will generate silent output when given silent input. By default, AudioNode
    // will take tailTime() and latencyTime() into account when determining whether the node will propagate silence.
    virtual bool propagatesSilence(ContextRenderLock & r) const;

    bool inputsAreSilent(ContextRenderLock&);
    void silenceOutputs(ContextRenderLock&);
    void unsilenceOutputs(ContextRenderLock&);

    size_t channelCount();
    virtual void setChannelCount(ContextGraphLock & g, size_t count);

    ChannelCountMode channelCountMode() const { return m_channelCountMode; }
    void setChannelCountMode(ContextGraphLock& g, ChannelCountMode mode);

    ChannelInterpretation channelInterpretation() const { return m_channelInterpretation; }
    void setChannelInterpretation(ChannelInterpretation interpretation) { m_channelInterpretation = interpretation; }

    // returns a vector of parameter names
    std::vector<std::string> params() const;

    // returns a vector of setting names
    std::vector<std::string> settings() const;

    // USER FACING FUNCTIONS >

    std::shared_ptr<AudioParam> getParam(char const * const str);
    std::shared_ptr<AudioSetting> getSetting(char const * const str);

    // USER FACING FUNCTIONS <
    
protected:

    // Inputs and outputs must be created before the AudioNode is initialized.
    // It is only legal to call this during a constructor.
    void addInput(std::unique_ptr<AudioNodeInput> input);
    void addOutput(std::unique_ptr<AudioNodeOutput> output);

    // Called by processIfNecessary() to cause all parts of the rendering graph connected to us to process.
    // Each rendering quantum, the audio data for each of the AudioNode's inputs will be available after this method is called.
    // Called from context's audio thread.
    virtual void pullInputs(ContextRenderLock&, size_t framesToProcess);

    // Force all inputs to take any channel interpretation changes into account.
    void updateChannelsForInputs(ContextGraphLock&);

private:

    friend class AudioContext;

    volatile bool m_isInitialized{ false };

    std::vector<std::shared_ptr<AudioNodeInput>> m_inputs;
    std::vector<std::shared_ptr<AudioNodeOutput>> m_outputs;

    double m_lastProcessingTime{ -1.0 };
    double m_lastNonSilentTime{ -1.0 };

    float audibleThreshold() const { return 0.05f; }

    // starts an immediate ramp to zero in preparation for disconnection
    void scheduleDisconnect()
    {
        m_disconnectSchedule = 1.f;
        m_connectSchedule = 1.f;
    }

    // returns true if the disconnection ramp has reached zero.
    // This is intended to allow the AudioContext to manage popping artifacts
    bool disconnectionReady() const { return m_disconnectSchedule >= 0.f && m_disconnectSchedule <= audibleThreshold(); }

    // starts an immediate ramp to unity due to being newly connected to a graph
    void scheduleConnect()
    {
        m_disconnectSchedule = -1.f;
        m_connectSchedule = 0.f;
    }

    // returns true if the connection has ramped to unity
    // This is intended to signal when the danger of possible popping artifacts has passed
    bool connectionReady() const { return m_connectSchedule > (1.f - audibleThreshold()); }

    std::atomic<float> m_disconnectSchedule{ -1.f };
    std::atomic<float> m_connectSchedule{ 0.f };

protected:

    std::vector<std::shared_ptr<AudioParam>> m_params;
    std::vector<std::shared_ptr<AudioSetting>> m_settings;
    size_t m_channelCount;
    float m_sampleRate;
    ChannelCountMode m_channelCountMode{ ChannelCountMode::Max };
    ChannelInterpretation m_channelInterpretation{ ChannelInterpretation::Speakers };
};

} // namespace lab

#endif // AudioNode_h
