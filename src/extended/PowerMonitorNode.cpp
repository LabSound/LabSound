// Copyright (c) 2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound/extended/PowerMonitorNode.h"

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/Macros.h"

namespace lab {
    
    using namespace lab;
    
    PowerMonitorNode::PowerMonitorNode() 
    : AudioBasicInspectorNode(2), _db(0)
    , _windowSize(std::make_shared<AudioSetting>("windowSize"))
    {
        addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this)));
        _windowSize->setUint32(128);
        m_settings.push_back(_windowSize);
        initialize();
    }
    
    PowerMonitorNode::~PowerMonitorNode()
    {
        uninitialize();
    }

    void PowerMonitorNode::windowSize(size_t ws)
    {
        _windowSize->setUint32(static_cast<uint32_t>(ws));
    }

    size_t PowerMonitorNode::windowSize() const
    {
        return _windowSize->valueUint32();
    }

    
    void PowerMonitorNode::process(ContextRenderLock& r, size_t framesToProcess)
    {
        // deal with the output in case the power monitor node is embedded in a signal chain for some reason.
        // It's merely a pass through though.
        
        AudioBus* outputBus = output(0)->bus(r);
        
        if (!isInitialized() || !input(0)->isConnected()) {
            if (outputBus)
                outputBus->zero();
            return;
        }
        
        AudioBus* bus = input(0)->bus(r);
        bool isBusGood = bus && bus->numberOfChannels() > 0 && bus->channel(0)->length() >= framesToProcess;
        if (!isBusGood) {
            outputBus->zero();
            return;
        }

        // specific to this node
        {
            std::vector<const float*> channels;
            size_t numberOfChannels = bus->numberOfChannels();
            for (size_t i = 0; i < numberOfChannels; ++i) {
                channels.push_back(bus->channel(i)->data());
            }

            int start = static_cast<int>(framesToProcess) - static_cast<int>(_windowSize->valueUint32());
            int end = static_cast<int>(framesToProcess);
            if (start < 0)
                start = 0;

            float power = 0;
            for (unsigned c = 0; c < numberOfChannels; ++c)
                for (int i = start; i < end; ++i) {
                    float p = channels[c][i];
                    power += p * p;
                }
            float rms = sqrtf(power / (numberOfChannels * framesToProcess));
            
            // Protect against accidental overload due to bad values in input stream
            const float kMinPower = 0.000125f;
            if (std::isinf(power) || std::isnan(power) || power < kMinPower)
                power = kMinPower;
            
            // db is 20 * log10(rms/Vref) where Vref is 1.0
            _db = 20.0f * logf(rms) / logf(10.0f);
        }
        // to here
        
        // For in-place processing, our override of pullInputs() will just pass the audio data
        // through unchanged if the channel count matches from input to output
        // (resulting in inputBus == outputBus). Otherwise, do an up-mix to stereo.
        //
        if (bus != outputBus)
            outputBus->copyFrom(*bus);
    }
    
    void PowerMonitorNode::reset(ContextRenderLock&)
    {
        _db = 0;
    }
    
} // namespace lab
