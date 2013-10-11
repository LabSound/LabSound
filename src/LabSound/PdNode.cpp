
// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "PdNode.h"
#include "ofxPd.h"

#include "AudioBus.h"
#include "AudioProcessor.h"

#include <iostream>

class PdNodeInternal : public WebCore::AudioProcessor, public pd::PdReceiver, public pd::PdMidiReceiver {
public:

    PdNodeInternal(float sampleRate)
    : AudioProcessor(sampleRate)
    , numChannels(1)
    , pdBuffSize(128)
    {
    }
    
    virtual ~PdNodeInternal()
    {
    }

    // AudioProcessor interface
    virtual void initialize() {
        initPure(numChannels, numChannels, sampleRate(), pdBuffSize);
    }
    
    virtual void uninitialize() { }
    
    // Processes the source to destination bus.  The number of channels must match in source and destination.
    virtual void process(const WebCore::AudioBus* source, WebCore::AudioBus* destination, size_t framesToProcess) {
        
        while (framesToProcess > 0) {
            pd.audioIn(source->channel(0)->data(), pdBuffSize, numChannels);
            pd.audioOut(destination->channel(0)->mutableData(), pdBuffSize, numChannels);
            framesToProcess -= pdBuffSize;
        }
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
    
    /// messages from PdReceiver
    virtual void receiveBang(const std::string& dest);
    virtual void receiveFloat(const std::string& dest, float num);
    virtual void receiveSymbol(const std::string& dest, const std::string& symbol);
    virtual void receiveList(const std::string& dest, const pd::List& list);
    virtual void receiveMessage(const std::string& dest, const std::string& msg, const pd::List& list);
    
    // messages from PdMidiReceiver
    virtual void receiveNoteOn(const int channel, const int pitch, const int velocity);
    virtual void receiveControlChange(const int channel, const int controller, const int value);
    virtual void receiveProgramChange(const int channel, const int value); // note: pgm value is 1-128
    virtual void receivePitchBend(const int channel, const int value);
    virtual void receiveAftertouch(const int channel, const int value);
    virtual void receivePolyAftertouch(const int channel, const int pitch, const int value);
    virtual void receiveMidiByte(const int port, const int byte);
    
    
    ofxPd pd;
    int numChannels;
    int pdBuffSize;
};


// return true on successful initialization
bool PdNodeInternal::initPure(const int numOutChannels, const int numInChannels,
                      const int sampleRate, const int ticksPerBuffer) {
    
    
    if (!pd.init(numOutChannels, numInChannels, sampleRate, ticksPerBuffer)) {
        return false;
    }
    
    // subscribe to receive source names
    pd.subscribe("toOF");
    pd.subscribe("env");
    
    // add message receiver, disables polling (see processEvents)
    pd.addReceiver(*this);   // automatically receives from all subscribed sources
    pd.ignore(*this, "env"); // don't receive from "env"
                             //pd.ignore(*this);             // ignore all sources
                             //pd.receive(*this, "toOF");	// receive only from "toOF"
    
    // add midi receiver
    pd.addMidiReceiver(*this);  // automatically receives from all channels
                                //pd.ignoreMidi(*this, 1);     // ignore midi channel 1
                                //pd.ignoreMidi(*this);        // ignore all channels
                                //pd.receiveMidi(*this, 1);    // receive only from channel 1
    
    // add the data/pd folder to the search path
    pd.addToSearchPath("pd");
    
    // audio processing on
    pd.start();
    
    return true;
}

//--------------------------------------------------------------
void PdNodeInternal::print(const std::string& message) {
    std::cout << message << std::endl;
}

//--------------------------------------------------------------
// PdReceiver methods
//
void PdNodeInternal::receiveBang(const std::string& dest) {
    std::cout << "OF: bang " << dest << std::endl;
}

void PdNodeInternal::receiveFloat(const std::string& dest, float value) {
	std::cout << "OF: float " << dest << ": " << value << std::endl;
}

void PdNodeInternal::receiveSymbol(const std::string& dest, const std::string& symbol) {
	std::cout << "OF: symbol " << dest << ": " << symbol << std::endl;
}

void PdNodeInternal::receiveList(const std::string& dest, const pd::List& list) {
	std::cout << "OF: list " << dest << ": ";
    
	// step through the list
	for(int i = 0; i < list.len(); ++i) {
		if(list.isFloat(i))
			std::cout << list.getFloat(i) << " ";
		else if(list.isSymbol(i))
			std::cout << list.getSymbol(i) << " ";
	}
    
	// you can also use the built in toString function or simply stream it out
	// std::cout << list.toString();
	// std::cout << list;
    
	// print an OSC-style type string
	std::cout << list.types() << std::endl;
}

void PdNodeInternal::receiveMessage(const std::string& dest, const std::string& msg, const pd::List& list) {
	std::cout << "OF: message " << dest << ": " << msg << " " << list.toString() << list.types() << std::endl;
}

//--------------------------------------------------------------
// PdMidiReceiver methods
//
void PdNodeInternal::receiveNoteOn(const int channel, const int pitch, const int velocity) {
	std::cout << "OF MIDI: note on: " << channel << " " << pitch << " " << velocity << std::endl;
}

void PdNodeInternal::receiveControlChange(const int channel, const int controller, const int value) {
	std::cout << "OF MIDI: control change: " << channel << " " << controller << " " << value << std::endl;
}

// note: pgm nums are 1-128 to match pd
void PdNodeInternal::receiveProgramChange(const int channel, const int value) {
	std::cout << "OF MIDI: program change: " << channel << " " << value << std::endl;
}

void PdNodeInternal::receivePitchBend(const int channel, const int value) {
	std::cout << "OF MIDI: pitch bend: " << channel << " " << value << std::endl;
}

void PdNodeInternal::receiveAftertouch(const int channel, const int value) {
	std::cout << "OF MIDI: aftertouch: " << channel << " " << value << std::endl;
}

void PdNodeInternal::receivePolyAftertouch(const int channel, const int pitch, const int value) {
	std::cout << "OF MIDI: poly aftertouch: " << channel << " " << pitch << " " << value << std::endl;
}

// note: pd adds +2 to the port num, so sending to port 3 in pd to [midiout],
//       shows up at port 1 in ofxPd
void PdNodeInternal::receiveMidiByte(const int port, const int byte) {
	std::cout << "OF MIDI: midi byte: " << port << " " << byte << std::endl;
}



PdNode::PdNode(WebCore::AudioContext* context, float sampleRate)
: WebCore::AudioBasicProcessorNode(context, sampleRate)
{
    // Initially setup as lowpass filter.
    m_processor = adoptPtr(new PdNodeInternal(sampleRate));
    setNodeType(NodeTypeBiquadFilter);
}

