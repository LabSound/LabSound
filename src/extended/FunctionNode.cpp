// Copyright (c) 2003-2015 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "LabSound/core/AudioNodeOutput.h"
#include "LabSound/extended/FunctionNode.h"
#include "internal/AudioBus.h"

using namespace std;
using namespace WebCore;

namespace LabSound {
    
    FunctionNode::FunctionNode(float sampleRate, int channels)
    : AudioScheduledSourceNode(sampleRate)
    , _now(0)
    {
        addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, channels)));
        initialize();
    }
    
    FunctionNode::~FunctionNode()
    {
        uninitialize();
    }
    
    void FunctionNode::process(ContextRenderLock& r, size_t framesToProcess)
    {
        AudioBus* outputBus = output(0)->bus(r);
        
        if (!isInitialized() || !outputBus->numberOfChannels() || !_function) {
            outputBus->zero();
            return;
        }
        
        size_t quantumFrameOffset;
        size_t nonSilentFramesToProcess;
        
        updateSchedulingInfo(r, framesToProcess, outputBus, quantumFrameOffset, nonSilentFramesToProcess);
        
        if (!nonSilentFramesToProcess) {
            outputBus->zero();
            return;
        }
        
        for (size_t i = 0; i < channelCount(); ++i) {
            float* destP = outputBus->channel(0)->mutableData();
            
            // Start rendering at the correct offset.
            destP += quantumFrameOffset;
            int n = nonSilentFramesToProcess;
            
            _function(r, this, i, destP, n);
        }

        _now += double(framesToProcess) / sampleRate();
        outputBus->clearSilentFlag();
    }
    
    void FunctionNode::reset(ContextRenderLock&)
    {
    }
    
    bool FunctionNode::propagatesSilence(double now) const
    {
        return !isPlayingOrScheduled() || hasFinished();
    }
    
} // namespace LabSound

