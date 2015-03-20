#include "OscillatorExample.h"

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
        oscillator->start(0);   // play now
        oscillator->frequency()->setValue(440.f);
        oscillator->setType(r, 1, ec);
	}

	std::this_thread::sleep_for(std::chrono::seconds(5));
    LabSound::finish(context);

    return 0;

}
