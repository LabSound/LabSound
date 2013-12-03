
#include "LabSound/LabSound.h"

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();


int main(int, char**)
{
    // Initialize threads for the WTF library
    WTF::initializeThreading();
    WTF::initializeMainThread();
    
    // Create an audio context object
    Document d;
    ExceptionCode ec;
    RefPtr<AudioContext> context = AudioContext::create(&d, ec);

    liveReverbRecording(context, 10.0f, "liveReverb.raw");
    
    return 0;
}
