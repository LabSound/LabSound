#include <iostream>
#include <chrono>
#include <thread>
#include "LabSound.h"
#include "LabSoundIncludes.h"

using namespace LabSound;

int main(int argc, char *argv[], char *envp[]) {

    WebCore::ExceptionCode ec;

	auto context = LabSound::init();

	std::shared_ptr<OscillatorNode> oscillator;
	std::shared_ptr<GainNode> limiter;

	{
        ContextGraphLock g(context, "Oscillator Example");
        ContextRenderLock r(context, "Oscillator Example");

		oscillator = std::make_shared<OscillatorNode>(r, context->sampleRate());

		limiter = std::make_shared<GainNode>(context->sampleRate());

		limiter->gain()->setValue(0.25f);
		oscillator->connect(context.get(), limiter.get(), 0, 0, ec); // Connect oscillator to gain

		limiter->connect(context.get(), context->destination().get(), 0, 0, ec); // connect gain to DAC
      
		oscillator->start(0);
        oscillator->setType(r, 0, ec);
	}

	float f = 220.f; 
	for (int s = 0; s < 3; s++)
	{
		f *= 2; 
		oscillator->frequency()->setValueAtTime(f, s);
	}

	std::this_thread::sleep_for(std::chrono::seconds(4));
    LabSound::finish(context);

    return 0;
}
