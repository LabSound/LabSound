// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioDestinationNode.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSourceProvider.h"
#include "LabSound/core/AudioBus.h"

#include "LabSound/extended/AudioContextLock.h"

#include "internal/AudioUtilities.h"
#include "internal/DenormalDisabler.h"
#include "internal/Assertions.h"

namespace lab 
{

    // LocalAudioInputProvider allows us to expose an AudioSourceProvider for local/live audio input.
    // If there is local/live audio input, we call set() with the audio input data every render quantum.
    class AudioDestinationNode::LocalAudioInputProvider : public AudioSourceProvider 
    {
        AudioBus m_sourceBus;

    public:

        LocalAudioInputProvider(size_t channelCount) : m_sourceBus(channelCount, AudioNode::ProcessingSizeInFrames)
        {
        }
        
        virtual ~LocalAudioInputProvider() 
        {        
        }

        void set(AudioBus* bus)
        {
            if (bus) m_sourceBus.copyFrom(*bus);
        }

        // AudioSourceProvider.
        virtual void provideInput(AudioBus * destinationBus, size_t numberOfFrames)
        {
            bool isGood = destinationBus && destinationBus->length() == numberOfFrames && m_sourceBus.length() == numberOfFrames;
            ASSERT(isGood);
            if (isGood) destinationBus->copyFrom(m_sourceBus);
        }

    };

    
AudioDestinationNode::AudioDestinationNode(AudioContext * context, size_t channelCount, float sampleRate) 
: m_currentSampleFrame(0), m_sampleRate(sampleRate), m_context(context)
{
    m_localAudioInputProvider = new LocalAudioInputProvider(channelCount);

    addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));

    // Node-specific default mixing rules.
    m_channelCount = channelCount;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Speakers;
    
    // NB: Special case - the audio context calls initialize so that rendering doesn't start before the context is ready
    // initialize();
}

AudioDestinationNode::~AudioDestinationNode()
{
    uninitialize();
}

void AudioDestinationNode::render(AudioBus * sourceBus, AudioBus * destinationBus, size_t numberOfFrames)
{
    // The audio system might still be invoking callbacks during shutdown, so bail out if so.
    if (!m_context)
        return;
    
    ContextRenderLock renderLock(m_context, "AudioDestinationNode::render");
    if (!renderLock.context())
        return; // return if couldn't acquire lock

    if (!m_context->isInitialized())
    {
        destinationBus->zero();
        return;
    }
    
    // Denormals can slow down audio processing. 
    /// @TODO under what circumstance do they arise?
    /// If they come from input data such as loaded WAV files, they should be corrected
    /// at source. If they can result from signal processing; again, where? The
    /// signal processing should not produce denormalized values.
    // Use an RAII object to protect all AudioNodes processed within this scope.
    DenormalDisabler denormalDisabler;

    // Let the context take care of any business at the start of each render quantum.
    m_context->handlePreRenderTasks(renderLock);

    // Prepare the local audio input provider for this render quantum.
    if (sourceBus)
        m_localAudioInputProvider->set(sourceBus);

    /// @TODO why is only input 0 processed?

    // process the graph by pulling the inputs, which will recurse the entire processing graph.
    AudioBus * renderedBus = input(0)->pull(renderLock, destinationBus, numberOfFrames);
    
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

double AudioDestinationNode::currentTime() const 
{ 
    return currentSampleFrame() / static_cast<double>(m_sampleRate); 
}

AudioSourceProvider * AudioDestinationNode::localAudioInputProvider() 
{ 
    return static_cast<AudioSourceProvider*>(m_localAudioInputProvider); 
}

} // namespace lab

