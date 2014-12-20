
#include "DalekVoice.h"
#include "LabSoundIncludes.h"

using namespace WebCore;

int main(int, char**)
{
    RefPtr<AudioContext> context = LabSound::init();
    dalekVoice(context, 30.0f);
    return 0;
}
