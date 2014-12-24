
#include "LabSoundIncludes.h"
#include "RhythmFiltered.h"

using namespace LabSound;

int main(int, char**) {
    {
        FILE* test = fopen("kick.wav", "rb");
        if (!test) {
            std::cerr << "Run demo in the examples data folder" << std::endl;
            return 1;
        }
        fclose(test);
    }
    
    auto context = LabSound::init();
    rhythmFiltered(context, 3.0f);
    LabSound::finish(context);
    return 0;
}
