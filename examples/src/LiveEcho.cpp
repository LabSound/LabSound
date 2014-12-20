
#include "LabSoundIncludes.h"
#include "LiveEcho.h"

using namespace LabSound;
void liveEcho(RefPtr<AudioContext> context, float seconds)
{
    ExceptionCode ec;
    //--------------------------------------------------------------
    // Send live audio straight to the output
    //
    RefPtr<MediaStreamAudioSourceNode> input = context->createMediaStreamSource(ec);
    input->connect(context->destination(), 0, 0, ec);
    std::cout << "Starting echo" << std::endl;
    
    while (seconds > 0) {
        seconds -= 0.01f;
        usleep(10000);
    }
    std::cout << "Ending echo" << std::endl;
}
