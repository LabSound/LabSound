#include "ProjectIncubator.h"
#include <cstdlib>
#include <ctime>

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
typedef RefPtr<LabSound::STKNode<stk::Sitar> > Sitar;
typedef RefPtr<LabSound::STKNode<stk::BlowBotl> > BlowBotl;

int main(int argc, char *argv[], char *envp[]) {

    WebCore::ExceptionCode ec;
	RefPtr<AudioContext> context = LabSound::init(); 

	// StifKarp myString = STKNode<stk::StifKarp>::create(context.get(), 44100);

	SoundBuffer tonbi(context, "tonbi.wav");
	tonbi.play(0.0f);

	// RefPtr<SampledInstrumentNode> sampledBell = 

	PassRefPtr<SampledInstrumentNode> test = SampledInstrumentNode::create(context.get(), 44100);

	for (int i = 0; i < 12; ++i) {

		test->noteOn(60 + i, 0.0); 
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

	}

    return 0;

}
