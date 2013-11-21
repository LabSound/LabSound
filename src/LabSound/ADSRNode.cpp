#include "ADSRNode.h"
#include "LabSound.h"

using namespace WebCore;
using namespace LabSound; 
using namespace std;

namespace LabSound {

ADSRNode::ADSRNode(AudioContext* context, float sampleRate) : GainNode(context, sampleRate) {
	m_attack  = AudioParam::create(context, "attack",  0.05, 0, 120);   // duration
	m_decay   = AudioParam::create(context, "decay",   0.1,  0, 120);   // duration
	m_sustain = AudioParam::create(context, "sustain", 0.75, 0, 120);   // level
	m_release = AudioParam::create(context, "release", 0.25, 0, 120);   // duration
    setNodeType((AudioNode::NodeType) LabSound::NodeTypeADSR);
    initialize();
}

ADSRNode::~ADSRNode() {

}

void ADSRNode::set(float a, float d, float s, float r) {

	m_attack->setValue(a);
	m_decay->setValue(d);
	m_sustain->setValue(s);
	m_release->setValue(r); 

}

void ADSRNode::noteOn() {

	double now = context()->currentTime(); 

	gain()->setValueAtTime(0, now); 

	// Attack
	gain()->linearRampToValueAtTime(1.0 , now + m_attack->value()); 
		
	now += m_attack->value();

	// Decay to sustain
	gain()->linearRampToValueAtTime(m_sustain->value(), now + m_decay->value()); 

}

void ADSRNode::noteOff() {
	
	double now = context()->currentTime(); 

    gain()->cancelScheduledValues(now);

	gain()->setValueAtTime(gain()->value(), now);

	// Release 
    gain()->linearRampToValueAtTime(0, now + m_release->value());

}

} // End namespace LabSound
