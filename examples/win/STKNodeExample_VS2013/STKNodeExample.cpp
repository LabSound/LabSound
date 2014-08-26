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

	Rhodey myRhodey = STKNode<stk::Rhodey>::create(context.get(), 44100);

	SoundBuffer tonbi(context, "tonbi.wav");

	tonbi.play(0.0f);

	myRhodey->connect(context->destination(), 0, 0, ec);

	myRhodey->start(0);

	stk::Rhodey gimmeSomeRhodey = myRhodey->getSynth();

    for (float i = 1; i < 6; i++) {

		gimmeSomeRhodey.noteOn(220 * i, 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		gimmeSomeRhodey.noteOff(32);
		std::this_thread::sleep_for(std::chrono::milliseconds(250));

    }

    return 0;

}
