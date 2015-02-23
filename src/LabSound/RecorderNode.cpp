// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSoundConfig.h"
#include "RecorderNode.h"
#include "LabSound.h"
#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"

namespace LabSound {
    
    using namespace WebCore;
    
    RecorderNode::RecorderNode(float sampleRate)
    : AudioBasicInspectorNode(sampleRate)
    , m_recording(false), m_mixToMono(false)
    {
        addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
        addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, 2)));
        
        setNodeType((AudioNode::NodeType) LabSound::NodeTypeRecorder);
        
        initialize();
    }
    
    RecorderNode::~RecorderNode()
    {
        uninitialize();
    }
    
    void RecorderNode::getData(std::vector<float>& result)
    {
        result.clear();
        // swap is quick enough that process should not be adversely affected
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        result.swap(m_data);
    }

    
    void RecorderNode::process(ContextRenderLock&, size_t framesToProcess)
    {
        AudioBus* outputBus = output(0)->bus();
        
        if (!isInitialized() || !input(0)->isConnected()) {
            if (outputBus)
                outputBus->zero();
            return;
        }

        // from here --- should this follow the WebAudio pattern have a writer object to call here?
        AudioBus* bus = input(0)->bus();
        bool isBusGood = bus && bus->numberOfChannels() > 0 && bus->channel(0)->length() >= framesToProcess;
        if (!isBusGood) {
            outputBus->zero();
            return;
        }
        
        if (m_recording) {
            std::vector<const float*> channels;
            unsigned numberOfChannels = bus->numberOfChannels();
			for (unsigned int c = 0; c < numberOfChannels; ++c)
                channels.push_back(bus->channel(c)->data());

            // mix down the output, or interleave the output
            // use the tightest loop possible since this is part of the processing step
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_data.reserve(framesToProcess * (m_mixToMono ? 1 : 2));
            if (m_mixToMono) {
                if (numberOfChannels == 1)
                    for (size_t i = 0; i < framesToProcess; ++i)
                        m_data.push_back(channels[0][i]);
                else
                    for (size_t i = 0; i < framesToProcess; ++i) {
                        float val = 0;
                        for (unsigned int c = 0; c < numberOfChannels; ++ c)
                            val += channels[c][i];
                        val *= 1.0f / float(numberOfChannels);
                        m_data.push_back(val);
                    }
            }
            else
                for (size_t i = 0; i < framesToProcess; ++i)
                    for (unsigned int c = 0; c < numberOfChannels; ++ c)
                        m_data.push_back(channels[c][i]);
        }
        // to here
        
        // For in-place processing, our override of pullInputs() will just pass the audio data
        // through unchanged if the channel count matches from input to output
        // (resulting in inputBus == outputBus). Otherwise, do an up-mix to stereo.
        //
        if (bus != outputBus)
            outputBus->copyFrom(*bus);
    }
    
    void RecorderNode::reset(std::shared_ptr<AudioContext>)
    {
        std::vector<float> clear;
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_data.swap(clear);
        }
        // release the data in clear's destructor after the mutex has been released
    }
                                           
} // namespace LabSound

