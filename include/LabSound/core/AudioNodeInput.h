// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioNodeInput_h
#define AudioNodeInput_h

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioSummingJunction.h"

#include <set>

namespace lab {

class AudioNode;
class AudioNodeOutput;
class AudioBus;

// An AudioNodeInput represents an input to an AudioNode and can be connected from one or more AudioNodeOutputs.
// In the case of multiple connections, the input will act as a unity-gain summing junction, mixing all the outputs.
// The number of channels of the input's bus is the maximum of the number of channels of all its connections.
class AudioNodeInput : public AudioSummingJunction 
{

public:

    explicit AudioNodeInput(AudioNode*);
    virtual ~AudioNodeInput();

    // AudioSummingJunction
    virtual bool canUpdateState() override { return !node()->isMarkedForDeletion(); }
    virtual void didUpdate(ContextRenderLock&) override;

    // Can be called from any thread.
    AudioNode* node() const { return m_node; }

    // Must be called with the context's graph lock. Static because a shared pointer to this is required
    static void connect(ContextGraphLock&, std::shared_ptr<AudioNodeInput> fromInput, std::shared_ptr<AudioNodeOutput> toOutput);
    static void disconnect(ContextGraphLock&, std::shared_ptr<AudioNodeInput> fromInput, std::shared_ptr<AudioNodeOutput> toOutput);

    // pull() processes all of the AudioNodes connected to us.
    // In the case of multiple connections it sums the result into an internal summing bus.
    // In the single connection case, it allows in-place processing where possible using inPlaceBus.
    // It returns the bus which it rendered into, returning inPlaceBus if in-place processing was performed.
    AudioBus* pull(ContextRenderLock&, AudioBus* inPlaceBus, size_t framesToProcess);

    // bus() contains the rendered audio after pull() has been called for each time quantum.
    AudioBus* bus(ContextRenderLock&);
    
    // updateInternalBus() updates m_internalSummingBus appropriately for the number of channels.
    // This must be called when we own the context's graph lock in the audio thread at the very start or end of the render quantum.
    void updateInternalBus(ContextRenderLock&);

    // The number of channels of the connection with the largest number of channels.
    // Only valid during render quantum because it is dependent on the active bus
    unsigned numberOfChannels(ContextRenderLock&) const;
    
private:

    AudioNode* m_node;

    // Called from context's audio thread.
    AudioBus * internalSummingBus(ContextRenderLock&);
    void sumAllConnections(ContextRenderLock&, AudioBus* summingBus, size_t framesToProcess);

    std::unique_ptr<AudioBus> m_internalSummingBus;
};

} // namespace lab

#endif // AudioNodeInput_h
