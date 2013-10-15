
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "PdNode.h"

#include "AudioBus.h"
#include "AudioProcessor.h"

#include "PdBase.hpp"
#include "z_libpd.h"

#include <iostream>

class PdNode::PdNodeInternal : public WebCore::AudioProcessor {
public:

    PdNodeInternal(float sampleRate)
    : AudioProcessor(sampleRate)
    , numChannels(1)
    , pdBuffSize(128)
    {
    }
    
    virtual ~PdNodeInternal() {
    }

    // AudioProcessor interface
    virtual void initialize() {
        initPure(numChannels, numChannels, sampleRate(), pdBuffSize);
    }
    
    virtual void uninitialize() { }
    
    // Processes the source to destination bus.  The number of channels must match in source and destination.
    virtual void process(const WebCore::AudioBus* source, WebCore::AudioBus* destination, size_t framesToProcess) {
        int ticks = framesToProcess >> blockSizeAsLog; // this is a faster way of computing (inNumberFrames / blockSize)
        pd.processFloat(ticks, (float*)source->channel(0)->data(), destination->channel(0)->mutableData());
    }
    
    // Resets filter state
    virtual void reset() { }
    
    virtual void setNumberOfChannels(unsigned i) {
        numChannels = i;
    }
    
    virtual double tailTime() const { return 0; }
    virtual double latencyTime() const { return 0; }
    
    // return true on successful initialization
    bool initPure(const int numOutChannels, const int numInChannels,
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
        int y = 0;
        while (x >>= 1) {
            ++y;
        }
        return y;
    }
} // anon


// return true on successful initialization
bool PdNode::PdNodeInternal::initPure(const int numOutChannels, const int numInChannels,
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
    m_processor = adoptPtr(new PdNodeInternal(sampleRate));
    setNodeType(NodeTypeBiquadFilter);
}

PdNode::~PdNode() {
    delete data;
}


