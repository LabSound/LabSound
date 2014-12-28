#include "LabSound.h"
#include "LabSoundIncludes.h"
#include <chrono>
#include <thread>

using namespace LabSound;

int main(int, char**)
{
    ExceptionCode ec;
    
    auto context = LabSound::init();
    
    auto input = context->createMediaStreamSource(context, ec);
    input->connect(context->destination().get(), 0, 0, ec);
    
    LabSound::finish(context);
    
    return 0;
}