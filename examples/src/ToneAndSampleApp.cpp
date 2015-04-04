#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;
using namespace std;

 // Play a tone and a sample at the same time
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    
    auto ac = context.get();
    
    std::shared_ptr<OscillatorNode> oscillator;
    SoundBuffer tonbi("samples/tonbi.wav", context->sampleRate());
    std::shared_ptr<AudioBufferSourceNode> tonbiSound;
    {
        ContextGraphLock g(context, "tone and sample");
        ContextRenderLock r(context, "tone and sample");
        oscillator = make_shared<OscillatorNode>(r, context->sampleRate());
        oscillator->connect(ac, context->destination().get(), 0, 0, ec);
        oscillator->start(0);
        oscillator->frequency()->setValue(440.f);
        oscillator->setType(r, 1, ec);
        tonbiSound = tonbi.play(r, 0.0f);
    }

    const int seconds = 2;
    for (int t = 0; t < seconds; ++t)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    LabSound::finish(context);
    return 0;
}
