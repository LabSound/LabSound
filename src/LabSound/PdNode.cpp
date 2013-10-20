
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "PdNode.h"

#include "AudioBus.h"
#include "AudioNodeInput.h"
#include "AudioNodeOutput.h"
#include "AudioProcessor.h"

#include "PdBase.hpp"
#include "z_libpd.h"

#include <iostream>

namespace LabSound {

class PdNode::PdNodeInternal : public WebCore::AudioProcessor {
public:

    PdNodeInternal(float sampleRate)
    : AudioProcessor(sampleRate)
    , numChannels(2)
    , pdBuffSize(128)
    {
    }
    
    virtual ~PdNodeInternal() {
    }

    // AudioProcessor interface
    virtual void initialize() {
    }
    
    virtual void uninitialize() { }
    
    // Processes the source to destination bus.  The number of channels must match in source and destination.
    virtual void process(const WebCore::AudioBus* source, WebCore::AudioBus* destination, size_t framesToProcess) {
        if (!numChannels)
            return;
        
        int ticks = framesToProcess >> blockSizeAsLog; // this is a faster way of computing (inNumberFrames / blockSize)
        const float* inData = source->channel(0)->data();
        float* outData = destination->channel(0)->mutableData();
        pd.processFloat(ticks, inData, outData);
    }
    
    // Resets filter state
    virtual void reset() { }
    
    virtual void setNumberOfChannels(unsigned i) {
        numChannels = i;
    }
    
    virtual double tailTime() const { return 0; }
    virtual double latencyTime() const { return 0; }
    
    // return true on successful initialization
    bool initPure(const int numInChannels, const int numOutChannels,
                  const int sampleRate, const int ticksPerBuffer);
    
    /// print
    virtual void print(const std::string& message);
    
    pd::PdBase pd;
    
    int numChannels;
    int pdBuffSize;
    int blockSizeAsLog;
};

namespace {
    //http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
    int log2int(int x) {
        if (x < 1) {
            return -1;
        }
        int y = 1;
        while (x >>= 1) {
            ++y;
        }
        return y;
    }
} // anon


// return true on successful initialization
bool PdNode::PdNodeInternal::initPure(const int numInChannels, const int numOutChannels,
                                      const int sampleRate, const int ticksPerBuffer) {
    
    if (!pd.init(numInChannels, numOutChannels, sampleRate)) {
        return false;
    }
    blockSizeAsLog = log2int(libpd_blocksize());
    
    // subscribe to receive source names
    pd.subscribe("toOF");
    pd.subscribe("env");
    
    // add the data/pd folder to the search path
    pd.addToSearchPath("pd");
    
    // audio processing on
    pd.computeAudio(true);
    
    return true;
}

//--------------------------------------------------------------
void PdNode::PdNodeInternal::print(const std::string& message) {
    std::cout << message << std::endl;
}

pd::PdBase& PdNode::pd() const {
    return data->pd;
}

PdNode::PdNode(WebCore::AudioContext* context, float sampleRate)
: WebCore::AudioBasicProcessorNode(context, sampleRate)
, data(new PdNodeInternal(sampleRate))
{
    // Initially setup as lowpass filter.
    m_processor = adoptPtr(data);
    
    setNodeType(NodeTypeConvolver);  // pretend to be a convolver
    
    addInput(adoptPtr(new WebCore::AudioNodeInput(this)));
    addOutput(adoptPtr(new WebCore::AudioNodeOutput(this, 2))); // 2 stereo

    data->initPure(2, 2, sampleRate, 128);
    
    initialize();
}

PdNode::~PdNode() {
    data->numChannels = 0;  // ensure there if there is a latent callback pending, pd is not invoked
    data = 0;
    uninitialize();
}

bool PdNode::propagatesSilence() const {
    return !data->pd.isInited();
}
    
    
} // LabSound

