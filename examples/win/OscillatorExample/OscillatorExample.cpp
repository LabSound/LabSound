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

	{
        ContextGraphLock g(context, "Oscillator Example");
        ContextRenderLock r(context, "Oscillator Example");
		oscillator = std::make_shared<OscillatorNode>(r, context->sampleRate());
		oscillator->connect(context.get(), context->destination().get(), 0, 0, ec);
        oscillator->start(0);
        oscillator->setType(r, 0, ec);

	}

	int count = 0; 
	for (int f = 220; f < 22050; f *= 2)
	{
		std::cout << f << std::endl;
		oscillator->frequency()->setValueAtTime(f, ++count);
	}

	std::this_thread::sleep_for(std::chrono::seconds(5));
    LabSound::finish(context);

    return 0;

}
