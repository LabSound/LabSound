#include "ADSRNode.h"

using namespace WebCore;
using namespace LabSound; 
using namespace std;

namespace LabSound {

ADSRNode::ADSRNode(AudioContext* context, float sampleRate) : GainNode(context, sampleRate) {

	WebCore::ExceptionCode ec;

	// Duration, Duration, Level, Duration 
	m_attack = AudioParam::create(context, "attack", 1.0, 0, 120);
	m_decay = AudioParam::create(context, "decay", 1.0, 0, 120);
	m_sustain = AudioParam::create(context, "sustain", 0.75, 0, 120);
	m_release = AudioParam::create(context, "release", 1.0, 0, 120);

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
