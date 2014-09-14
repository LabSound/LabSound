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

std::array<int, 8> majorScale = {0, 2, 4, 5, 7, 9, 11, 12};
std::array<int, 8> naturalMinorScale = {0, 2, 3, 5, 7, 9, 11, 12}; 
std::array<int, 6> pentatonicMajor = { 0, 2, 4, 7, 9, 12 };
std::array<int, 8> pentatonicMinor = { 0, 3, 5, 7, 10, 12 };

std::array<int, 8> delayTimes = { 266, 533, 399 };

int main(int argc, char *argv[], char *envp[]) {

	std::mt19937 mt_rand;
	auto randomFloat = std::bind(std::uniform_real_distribution<float>(0, 1), std::ref(mt_rand));
	auto randomScaleDegree = std::bind(std::uniform_int_distribution<int>(0, pentatonicMajor.size() - 1), std::ref(mt_rand));
	auto randomOctave = std::bind(std::uniform_int_distribution<int>(1, 4), std::ref(mt_rand));
	std::normal_distribution<> randomTime(0, 1); 

    WebCore::ExceptionCode ec;
	RefPtr<AudioContext> context = LabSound::init(); 

	RefPtr<EasyVerbNode> perryNode = EasyVerbNode::create(context.get(), 44100); 

	PassRefPtr<SampledInstrumentNode> sampledCello = SampledInstrumentNode::create(context.get(), 44100);
	sampledCello->loadInstrumentConfiguration("cello_pluck.json");

	// Connect internal output to the reverb
	sampledCello->gainNode->connect(perryNode.get(), 0, 0, ec);

	// Perry to the master bus 
	perryNode->connect(context->destination(), 0, 0, ec); 

	// 0.5 Second Verb! 
	perryNode->delayTime()->setValue(0.5);

	// Need to schedule ahead
	for (int i = 0; i < 1000; ++i) {

		auto delayTime = delayTimes[std::abs((int)randomTime(mt_rand))];

		sampledCello->noteOn((randomOctave() * 12) + pentatonicMinor[randomScaleDegree()], 0.5);

		std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));

	}

    return 0;

}
