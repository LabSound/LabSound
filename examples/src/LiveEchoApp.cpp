#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;
using namespace std;

int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    shared_ptr<MediaStreamAudioSourceNode> input;

    {
        ContextGraphLock g(context, "live echo");
        ContextRenderLock r(context, "live echo");
        input = context->createMediaStreamSource(g,r, ec);
        input->connect(context.get(), context->destination().get(), 0, 0, ec);
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(10));
    LabSound::finish(context);
    
    return 0;
}
