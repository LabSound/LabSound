// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioDestinationNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSourceProvider.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioUtilities.h"
#include "internal/DenormalDisabler.h"

#include "internal/AudioBus.h"

namespace lab 
{

    // LocalAudioInputProvider allows us to expose an AudioSourceProvider for local/live audio input.
    // If there is local/live audio input, we call set() with the audio input data every render quantum.
    class AudioDestinationNode::LocalAudioInputProvider : public AudioSourceProvider 
    {
    public:
        LocalAudioInputProvider() : m_sourceBus(2, AudioNode::ProcessingSizeInFrames) // FIXME: handle non-stereo local input.
        {

        }
        
        virtual ~LocalAudioInputProvider() 
        {
        
        }

        void set(AudioBus* bus)
        {
            if (bus)
                m_sourceBus.copyFrom(*bus);
        }

        // AudioSourceProvider.
        virtual void provideInput(AudioBus* destinationBus, size_t numberOfFrames)
        {
            bool isGood = destinationBus && destinationBus->length() == numberOfFrames && m_sourceBus.length() == numberOfFrames;
            ASSERT(isGood);
            if (isGood)
                destinationBus->copyFrom(m_sourceBus);
        }

    private:
        AudioBus m_sourceBus;
    };

    
AudioDestinationNode::AudioDestinationNode(std::shared_ptr<AudioContext> c, float sampleRate) : AudioNode(sampleRate) , m_currentSampleFrame(0), m_context(c)
{
    m_localAudioInputProvider = new LocalAudioInputProvider();

    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
    setNodeType(NodeTypeDestination);

    // Node-specific default mixing rules.
    m_channelCount = 2;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Speakers;
    
    // NB: Special case - the audio context calls initialize so that rendering doesn't start before the context is ready
    // initialize();
}

AudioDestinationNode::~AudioDestinationNode()
{
    uninitialize();
}

void AudioDestinationNode::render(AudioBus* sourceBus, AudioBus* destinationBus, size_t numberOfFrames)
{
    // The audio system might still be invoking callbacks during shutdown, so bail out if so.
    if (!m_context)
        return;
    
    // We don't want denormals slowing down any of the audio processing
    // since they can very seriously hurt performance.
    // This will take care of all AudioNodes because they all process within this scope.
    DenormalDisabler denormalDisabler;
    
    ContextRenderLock renderLock(m_context, "AudioDestinationNode::render");
    if (!renderLock.context())
        return; // return if couldn't acquire lock
    
    if (!m_context->isInitialized())
    {
        destinationBus->zero();
        return;
    }

    // Let the context take care of any business at the start of each render quantum.
    m_context->handlePreRenderTasks(renderLock);

    // Prepare the local audio input provider for this render quantum.
    if (sourceBus)
        m_localAudioInputProvider->set(sourceBus);

    // This will cause the node(s) connected to this destination node to process, which in turn will pull on their input(s),
    // all the way backwards through the rendering graph.
    AudioBus* renderedBus = input(0)->pull(renderLock, destinationBus, numberOfFrames);
    
    if (!renderedBus)
    {
        destinationBus->zero();
    }
    else if (renderedBus != destinationBus)
    {
        // in-place processing was not possible - so copy
        destinationBus->copyFrom(*renderedBus);
    }

    // Process nodes which need a little extra help because they are not connected to anything, but still need to process.
    m_context->processAutomaticPullNodes(renderLock, numberOfFrames);

    // Let the context take care of any business at the end of each render quantum.
    m_context->handlePostRenderTasks(renderLock);
    
    // Advance current sample-frame.
    m_currentSampleFrame += numberOfFrames;
}

AudioSourceProvider * AudioDestinationNode::localAudioInputProvider() 
{ 
    return static_cast<AudioSourceProvider*>(m_localAudioInputProvider); 
}

} // namespace lab

