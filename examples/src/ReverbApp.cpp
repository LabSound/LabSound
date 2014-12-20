
#include "LabSoundIncludes.h"

// Examples
#include "ReverbSample.h"
#include <iostream>

using namespace LabSound;

int main(int, char**) {
    {
        FILE* test = fopen("human-voice.mp4", "rb");
        if (!test) {
            std::cerr << "Run demo in the examples data folder" << std::endl;
            return 1;
        }
        fclose(test);
    }
    
    auto context = LabSound::init();
    reverbSample(context, 10.0f);
    return 0;
}
