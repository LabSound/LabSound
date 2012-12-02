
#include "AudioContext.h"
#include "ExceptionCode.h"
#include "MainThread.h"
#include "OscillatorNode.h"

#include <unistd.h>

using namespace WebCore;

namespace WebCore
{
    class Document { public: };
}

int main(int, char**)
{
    // Initialize threads for the WTF library
    WTF::initializeThreading();
    WTF::initializeMainThread();
    
    // Create an audio context object
    Document d;
    ExceptionCode ec;
    PassRefPtr<AudioContext> context = AudioContext::create(&d, ec);

    PassRefPtr<OscillatorNode> oscillator = context->createOscillator();
    oscillator->connect(context->destination(), 0, 0, ec);
    oscillator->start(0);
    
    for (int i = 0; i < 300; ++i)
        usleep(10000);
    
    return 0;
}
