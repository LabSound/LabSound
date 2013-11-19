

// For starting the WTF library
#include <wtf/ExportMacros.h>
#include <wtf/MainThread.h>

// Examples
#include "SampleSpatialization.h"


using namespace WebCore;
using LabSound::RecorderNode;

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
    RefPtr<AudioContext> context = AudioContext::create(&d, ec);

    sampleSpatialization(context, 10.0f);

    return 0;
}
