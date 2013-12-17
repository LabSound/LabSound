
// Copyright (c) 2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "PowerMonitorNode.h"
#include "LabSound.h"

#if ENABLE(WEB_AUDIO)

#include "RecorderNode.h"

#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "ExceptionCode.h"

namespace LabSound {
    
    using namespace WebCore;
    
    PowerMonitorNode::PowerMonitorNode(AudioContext* context, float sampleRate)
    : AudioBasicInspectorNode(context, sampleRate)
    , _db(0)
    , _windowSize(128)
    {
        addInput(adoptPtr(new AudioNodeInput(this)));
        setNodeType((AudioNode::NodeType) NodeTypePowerMonitor);
        initialize();
    }
    
    PowerMonitorNode::~PowerMonitorNode()
    {
        uninitialize();
    }
    
    void PowerMonitorNode::process(size_t framesToProcess)
    {
        // deal with the output in case the power monitor node is embedded in a signal chain for some reason.
        // It's merely a pass through though.
        
        AudioBus* outputBus = output(0)->bus();
        
        if (!isInitialized() || !input(0)->isConnected()) {
            if (outputBus)
                outputBus->zero();
            return;
        }
        
        AudioBus* bus = input(0)->bus();
        bool isBusGood = bus && bus->numberOfChannels() > 0 && bus->channel(0)->length() >= framesToProcess;
        if (!isBusGood) {
            outputBus->zero();
            return;
        }

        // specific to this node
        {
            std::vector<const float*> channels;
            unsigned numberOfChannels = bus->numberOfChannels();
            for (int c = 0; c < numberOfChannels; ++ c)
                channels.push_back(bus->channel(c)->data());

            int start = framesToProcess - _windowSize;
            int end = framesToProcess;
            if (start < 0)
                start = 0;

            float power = 0;
            for (int c = 0; c < numberOfChannels; ++c)
                for (int i = start; i < end; ++i) {
                    float p = channels[c][i];
                    power += p * p;
                }
            float rms = sqrtf(power / (numberOfChannels * framesToProcess));
            
            // Protect against accidental overload due to bad values in input stream
            const float kMinPower = 0.000125f;
            if (isinf(power) || isnan(power) || power < kMinPower)
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
    
    void PowerMonitorNode::reset()
    {
        _db = 0;
    }
    
} // namespace LabSound

#endif // ENABLE(WEB_AUDIO)
