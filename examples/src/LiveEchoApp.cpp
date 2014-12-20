
#include "LabSoundIncludes.h"
#include "LiveEcho.h"

int main(int, char**) {
    RefPtr<LabSound::AudioContext> context = LabSound::init();
    liveEcho(context, 3.0f);
    return 0;
}
