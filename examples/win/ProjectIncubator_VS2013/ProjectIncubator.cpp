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

	std::mt19937 mt_rand;
	auto randomFloat = std::bind(std::uniform_real_distribution<float>(0, 1), std::ref(mt_rand));

    WebCore::ExceptionCode ec;
	RefPtr<AudioContext> context = LabSound::init(); 

	SoundBuffer ir(context, "cardiod-rear-levelled.wav");


	RefPtr<WebCore::AudioBuffer> fakeImpulse;
	fakeImpulse = context->createBuffer(1, 256, 44100, ec);

	float* mutableImpulse = fakeImpulse->getChannelData(0)->data();

	// Decay = 0, 100
	//  impulseL[i] = (Math.random() * 2 - 1) * Math.pow(1 - n / length, decay);
	std::cout << "Impulse Length: " << fakeImpulse->length() << std::endl;

	auto impulse = [](float k, float x) {
		const float h = k*x;
		return h*expf(1.0f - h);
	};

	for (int i = 1; i <= fakeImpulse->length(); ++i) {
		mutableImpulse[i] = impulse(0.95, i);
	}

	RefPtr<ConvolverNode> convolve = context->createConvolver();

	convolve->setBuffer(ir.audioBuffer.get());


	RefPtr<EasyVerbNode> perryNode = EasyVerbNode::create(context.get(), 44100); 

	PassRefPtr<SampledInstrumentNode> sampledCello = SampledInstrumentNode::create(context.get(), 44100);
	sampledCello->loadInstrumentConfiguration("cello_pluck.json");

	// Connect internal to the reverb
	sampledCello->gainNode->connect(perryNode.get(), 0, 0, ec);

	// Perry to the master bus 
	perryNode->connect(context->destination(), 0, 0, ec); 

	// 8 Second Verb! 
	perryNode->delayTime()->setValue(2);

	for (int i = 0; i < 46; ++i) {


		std::cout << i << std::endl; 
		sampledCello->noteOn(i, 0.5);
		std::this_thread::sleep_for(std::chrono::milliseconds(250));

	}

    return 0;

}
