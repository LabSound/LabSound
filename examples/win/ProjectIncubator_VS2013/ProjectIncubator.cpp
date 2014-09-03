#include "ProjectIncubator.h"

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
typedef RefPtr<LabSound::STKNode<stk::Sitar> > Sitar;
typedef RefPtr<LabSound::STKNode<stk::BlowBotl> > BlowBotl;

#include <cstdlib>
#include <ctime>

int main(int argc, char *argv[], char *envp[]) {

    WebCore::ExceptionCode ec;
	RefPtr<AudioContext> context = LabSound::init(); 

	StifKarp myString = STKNode<stk::StifKarp>::create(context.get(), 44100);

	SoundBuffer tonbi(context, "tonbi.wav");
	// tonbi.play(0.0f);

	myString->connect(context->destination(), 0, 0, ec);

	myString->start(0);

	stk::StifKarp pluckedString = myString->getSynth();

	// pluckedString.setPreset(3);

    for (float i = 1; i < 100; i++) {

		// 0 to 1
		float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

		// pluckedString.noteOn(880, 0.75);

		//pluckedString.noteOn(220 * i, 1);
		// pluckedString.setPluckPosition(0.65);
		//pluckedString.setStretch(0.95);
		pluckedString.noteOn(880, 0.5);

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));


    }

    return 0;

}
