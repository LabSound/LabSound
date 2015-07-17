/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

#define DEBUG_AUDIONODE_REFERENCES 1

namespace LabSound 
{
    class ContextGraphLock;
    class ContextRenderLock;

	enum PanningMode
	{
        EQUALPOWER = 10,
        HRTF = 20,
    };

	enum NodeType 
	{
        // Core Webaudio Nodes
        NodeTypeUnknown,
        NodeTypeDestination,
        NodeTypeOscillator,
        NodeTypeAudioBufferSource,
        NodeTypeAudio,
        NodeTypeHardwareSource,
        NodeTypeBiquadFilter,
        NodeTypePanner,
        NodeTypeConvolver,
        NodeTypeDelay,
        NodeTypeGain,
        NodeTypeChannelSplitter,
        NodeTypeChannelMerger,
        NodeTypeAnalyser,
        NodeTypeDynamicsCompressor,
        NodeTypeWaveShaper,

        // Labsound Extensions
        NodeTypeADSR,
        NodeTypeClip,
        NodeTypeDiode,
        NodeTypeNoise,
        NodeTypePd,
        NodeTypePeakComp,
        NodeTypePowerMonitor,
        NodeTypePWM,
        NodeTypeRecorder,
        NodeTypeSfxr,
        NodeTypeSpatialization,
        NodeTypeSpectralMonitor,
        NodeTypeSupersaw,
		NodeTypeSTK, 

        // enumeration terminator
        NodeTypeEnd,
    };

}

namespace WebCore 
{
  
using namespace LabSound;

class AudioContext;
class AudioNodeInput;
class AudioNodeOutput;
class AudioParam;

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
    
    AudioNode(float sampleRate);
    virtual ~AudioNode();

    //@tofix these are defined in multiple namespaces (labsound vs webcore)
    enum NodeType 
	{
        NodeTypeUnknown,
        NodeTypeDestination,
        NodeTypeOscillator,
        NodeTypeAudioBufferSource,
        NodeTypeHardwareSource,
        NodeTypeBiquadFilter,
        NodeTypeStereoPanner,
        NodeTypePanner,
        NodeTypeConvolver,
        NodeTypeDelay,
        NodeTypeGain,
        NodeTypeChannelSplitter,
        NodeTypeChannelMerger,
        NodeTypeAnalyser,
        NodeTypeDynamicsCompressor,
        NodeTypeWaveShaper,
        NodeTypeEnd
    };
    
    NodeType nodeType() const { return m_nodeType; }
    void setNodeType(NodeType);

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
    void lazyInitialize();

    unsigned int numberOfInputs() const { return (unsigned int) m_inputs.size(); }
    unsigned int numberOfOutputs() const { return (unsigned int) m_outputs.size(); }

    std::shared_ptr<AudioNodeInput> input(unsigned);
    std::shared_ptr<AudioNodeOutput> output(unsigned);

    void connect(AudioContext*, AudioNode*, unsigned outputIndex, unsigned inputIndex);
    void connect(ContextGraphLock& g, std::shared_ptr<AudioParam>, unsigned outputIndex);
    
    void disconnect(unsigned outputIndex);

    virtual float sampleRate() const { return m_sampleRate; }

    // processIfNecessary() is called by our output(s) when the rendering graph needs this AudioNode to process.
    // This method ensures that the AudioNode will only process once per rendering time quantum even if it's called repeatedly.
    // This handles the case of "fanout" where an output is connected to multiple AudioNode inputs.
    // Called from context's audio thread.
    void processIfNecessary(ContextRenderLock& r, size_t framesToProcess);
    
    // @TODO webkit change 1f083e8 and 2bd2dc2 adds support for different behaviors on mixing such as
    // clamping the max number of channels, and mixing 5.1 down to mono

    // Called when a new connection has been made to one of our inputs or the connection number of channels has changed.
    // This potentially gives us enough information to perform a lazy initialization or, if necessary, a re-initialization.
    // Called from main thread.
    virtual void checkNumberOfChannelsForInput(ContextRenderLock&, AudioNodeInput*);

#if DEBUG_AUDIONODE_REFERENCES
    static void printNodeCounts();
#endif

    bool isMarkedForDeletion() const { return m_isMarkedForDeletion; }

    // tailTime() is the length of time (not counting latency time) where non-zero output may occur after continuous silent input.
    virtual double tailTime() const = 0;
    // latencyTime() is the length of time it takes for non-zero output to appear after non-zero input is provided. This only applies to
    // processing delay which is an artifact of the processing algorithm chosen and is *not* part of the intrinsic desired effect. For 
    // example, a "delay" effect is expected to delay the signal, and thus would not be considered latency.
    virtual double latencyTime() const = 0;

    // propagatesSilence() should return true if the node will generate silent output when given silent input. By default, AudioNode
    // will take tailTime() and latencyTime() into account when determining whether the node will propagate silence.
    virtual bool propagatesSilence(double now) const;
    bool inputsAreSilent(ContextRenderLock&);
    void silenceOutputs(ContextRenderLock&);
    void unsilenceOutputs(ContextRenderLock&);

    unsigned long channelCount();
    virtual void setChannelCount(ContextGraphLock&, unsigned long);

    ChannelCountMode channelCountMode() const { return m_channelCountMode; }
    void setChannelCountMode(ContextGraphLock& g, ChannelCountMode mode);

    ChannelInterpretation channelInterpretation() const { return m_channelInterpretation; }
    void setChannelInterpretation(ChannelCountMode);

protected:
    
    // Inputs and outputs must be created before the AudioNode is initialized.
    // It is only legal to call this during a constructor.
    void addInput(std::unique_ptr<AudioNodeInput>);
    void addOutput(std::unique_ptr<AudioNodeOutput>);
    
    // Called by processIfNecessary() to cause all parts of the rendering graph connected to us to process.
    // Each rendering quantum, the audio data for each of the AudioNode's inputs will be available after this method is called.
    // Called from context's audio thread.
    virtual void pullInputs(ContextRenderLock&, size_t framesToProcess);
    
    // Force all inputs to take any channel interpretation changes into account.
    void updateChannelsForInputs(ContextGraphLock&);
    
private:
    friend class AudioContext;
    
    volatile bool m_isInitialized;
    NodeType m_nodeType;
    float m_sampleRate;

    std::vector<std::shared_ptr<AudioNodeInput>> m_inputs;
    std::vector<std::shared_ptr<AudioNodeOutput>> m_outputs;

    double m_lastProcessingTime;
    double m_lastNonSilentTime;

    // Ref-counting
    std::atomic<int> m_connectionRefCount;
    
    bool m_isMarkedForDeletion;
    
#if DEBUG_AUDIONODE_REFERENCES
    static bool s_isNodeCountInitialized;
    static int s_nodeCount[NodeTypeEnd];
#endif
    
protected:
    unsigned m_channelCount;
    ChannelCountMode m_channelCountMode;
    ChannelInterpretation m_channelInterpretation;
};

} // namespace WebCore

#endif // AudioNode_h
