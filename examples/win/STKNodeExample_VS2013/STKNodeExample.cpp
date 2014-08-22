#include "STKNodeExample.h"

#include "LabSound/LabSound.h"
#include "LabSound/LabSoundIncludes.h"

using namespace LabSound;

typedef RefPtr<LabSound::STKNode<stk::BeeThree> > BeeThree;
typedef RefPtr<LabSound::STKNode<stk::Rhodey> > Rhodey;
typedef RefPtr<LabSound::STKNode<stk::Wurley> > Wurley;
typedef RefPtr<LabSound::STKNode<stk::TubeBell> > TubeBell;
typedef RefPtr<LabSound::STKNode<stk::Mandolin> > Mandolin;
typedef RefPtr<LabSound::STKNode<stk::ModalBar> > ModalBar;
typedef RefPtr<LabSound::STKNode<stk::Moog> > Moog;
typedef RefPtr<LabSound::STKNode<stk::Shakers> > Shakers;
typedef RefPtr<LabSound::STKNode<stk::StifKarp> > StifKarp;

int main(int argc, char *argv[], char *envp[]) {

    WebCore::ExceptionCode ec;
	RefPtr<AudioContext> context = LabSound::init(); 

	BeeThree myTB303 = STKNode<stk::BeeThree>::create(context.get(), context->sampleRate());

	myTB303->connect(context->destination(), 0, 0, ec);

	myTB303->start(0);

    for (float i = 0; i < 20; i += 0.1f) {

		std::cout << "Note on! " << i * 10 << std::endl; 

		myTB303->getSynth().noteOn(i * 10, 0.25);
		std::this_thread::sleep_for(std::chrono::milliseconds(250));

		//beeThree->noteOff();
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    }

    return 0;

}
