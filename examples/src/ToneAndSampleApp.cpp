#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;

 // Play a tone and a sample at the same time
int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();

    auto oscillator = OscillatorNode::create(context, context->sampleRate());
    oscillator->connect(context->destination().get(), 0, 0, ec);
    oscillator->start(0);   // play now
    oscillator->frequency()->setValue(440.f);
    oscillator->setType(1, ec);
    
    SoundBuffer tonbi(context, "tonbi.wav");
    tonbi.play(0.0f);

    const int seconds = 10;
    for (int t = 0; t < seconds; ++t)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    LabSound::finish(context);
    
    return 0;
}