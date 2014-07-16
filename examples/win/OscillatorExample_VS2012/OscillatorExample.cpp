#include "OscillatorExample.h"

#include "LabSound/LabSound.h"
#include "LabSound/LabSoundIncludes.h"

using namespace LabSound;

int main(int argc, char *argv[], char *envp[]) {


    WebCore::ExceptionCode ec;
	RefPtr<AudioContext> context = LabSound::init(); 

	RefPtr<OscillatorNode> oscillator = context->createOscillator();

	 SoundBuffer tonbi(context, "tonbi.wav");

	oscillator->connect(context->destination(), 0, 0, ec);
	oscillator->start(0);
	oscillator->setType(0, ec); // Default SinOsc
	oscillator->frequency()->setValue(440.f); 

	tonbi.play(context->destination(), 0.0f);

	std::this_thread::sleep_for(std::chrono::seconds(6));

    return 0;

}
