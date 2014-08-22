#include "STKNodeExample.h"

#include "LabSound/LabSound.h"
#include "LabSound/LabSoundIncludes.h"

using namespace LabSound;

int main(int argc, char *argv[], char *envp[]) {

    WebCore::ExceptionCode ec;
	RefPtr<AudioContext> context = LabSound::init(); 

	RefPtr<STKNode> beeThree =  STKNode::create(context.get(), context->sampleRate());

	beeThree->connect(context->destination(), 0, 0, ec);

	beeThree->start(0);

    for (float i = 0; i < 20; i += 0.1f) {

		std::cout << "Note on! " << i * 10 << std::endl; 

		beeThree->noteOn(i * 10); 
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		//beeThree->noteOff();
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    }

    return 0;

}
