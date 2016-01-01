// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioNodeOutput_h
#define AudioNodeOutput_h

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"

#include <set>

namespace lab {

class AudioContext;
class AudioNodeInput;
class AudioBus;

// AudioNodeOutput represents a single output for an AudioNode.
// It may be connected to one or more AudioNodeInputs.
class AudioNodeOutput 
{
public:

    // It's OK to pass 0 for numberOfChannels in which case setNumberOfChannels() must be called later on.
    AudioNodeOutput(AudioNode*, unsigned numberOfChannels);
    virtual ~AudioNodeOutput();

    // Can be called from any thread.
    AudioNode * node() const { return m_node; }
    
    // Causes our AudioNode to process if it hasn't already for this render quantum.
    // It returns the bus containing the processed audio for this output, returning inPlaceBus if in-place processing was possible.
    // Called from context's audio thread.
    AudioBus * pull(ContextRenderLock&, AudioBus* inPlaceBus, size_t framesToProcess);

    // bus() will contain the rendered audio after pull() is called for each rendering time quantum.
    AudioBus * bus(ContextRenderLock&) const;

    // renderingFanOutCount() is the number of AudioNodeInputs that we're connected to during rendering.
    // Unlike fanOutCount() it will not change during the course of a render quantum.
    unsigned renderingFanOutCount() const;

    // renderingParamFanOutCount() is the number of AudioParams that we're connected to during rendering.
    // Unlike paramFanOutCount() it will not change during the course of a render quantum.
    unsigned renderingParamFanOutCount() const;

    void setNumberOfChannels(ContextRenderLock&, unsigned);
    unsigned numberOfChannels() const { return m_numberOfChannels; }
    bool isChannelCountKnown() const { return numberOfChannels() > 0; }

    bool isConnected() { return fanOutCount() > 0 || paramFanOutCount() > 0; }
    
    // updateRenderingState() is called in the audio thread at the start or end of the render quantum to handle any recent changes to the graph state.
    void updateRenderingState(ContextRenderLock&);

    // Must be called within the context's graph lock.
    static void disconnectAll(ContextGraphLock &, std::shared_ptr<AudioNodeOutput>);
    static void disconnectAllInputs(ContextGraphLock&, std::shared_ptr<AudioNodeOutput>);
    static void disconnectAllParams(ContextGraphLock&, std::shared_ptr<AudioNodeOutput>);

private:

    AudioNode * m_node;

    friend class AudioNodeInput;
    friend class AudioParam;
    
    // These are called from AudioNodeInput.
    // They must be called with the context's graph lock.
    void addInput(ContextGraphLock& g, std::shared_ptr<AudioNodeInput>);
    void removeInput(ContextGraphLock& g, std::shared_ptr<AudioNodeInput>);
    void addParam(ContextGraphLock& g, std::shared_ptr<AudioParam>);
    void removeParam(ContextGraphLock& g, std::shared_ptr<AudioParam>);

    // fanOutCount() is the number of AudioNodeInputs that we're connected to.
    // This method should not be called in audio thread rendering code, instead renderingFanOutCount() should be used.
    // It must be called with the context's graph lock.
    unsigned fanOutCount();

    // Similar to fanOutCount(), paramFanOutCount() is the number of AudioParams that we're connected to.
    // This method should not be called in audio thread rendering code, instead renderingParamFanOutCount() should be used.
    // It must be called with the context's graph lock.
    unsigned paramFanOutCount();

    // updateInternalBus() updates m_internalBus appropriately for the number of channels.
    // It is called in the constructor or in the audio thread with the context's graph lock.
    void updateInternalBus();

    // Announce to any nodes we're connected to that we changed our channel count for its input.
    void propagateChannelCount(ContextRenderLock&);

    // m_numberOfChannels will only be changed in the audio thread.
    // The main thread sets m_desiredNumberOfChannels which will later get picked up in the audio thread
    unsigned m_numberOfChannels;
    unsigned m_desiredNumberOfChannels;
    
    // m_internalBus and m_inPlaceBus must only be changed in the audio thread with the context's render lock (or constructor).
    std::unique_ptr<AudioBus> m_internalBus;
    
    // Temporary, during render quantum
    // @tofix - Should this be some kind of shared pointer? It is only valid for a single render quantum, so probably no.
    AudioBus* m_inPlaceBus;
    
    std::vector<std::shared_ptr<AudioNodeInput>> m_inputs;
    
private:
    
    // If m_isInPlace is true, use m_inPlaceBus as the valid AudioBus; If false, use the default m_internalBus.
    bool m_isInPlace;
    
    // For the purposes of rendering, keeps track of the number of inputs and AudioParams we're connected to.
    // These value should only be changed at the very start or end of the rendering quantum.
    unsigned m_renderingFanOutCount;
    unsigned m_renderingParamFanOutCount;

    std::set<std::shared_ptr<AudioParam>> m_params;
    typedef std::set<AudioParam*>::iterator ParamsIterator;
};

} // namespace lab

#endif // AudioNodeOutput_h
