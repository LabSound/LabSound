
// Using soundpipe in LabSound
#if 0
typedef struct {
    sp_diskin *diskin;
    sp_conv *conv;
    sp_ftbl *ft;
} UserData;

void process(sp_data *sp, void *udata) {
    UserData *ud = udata;
    SPFLOAT conv = 0, diskin = 0, bal = 0;
    sp_diskin_compute(sp, ud->diskin, NULL, &diskin);
    sp_conv_compute(sp, ud->conv, &diskin, &conv);
    sp->out[0] = conv * 0.05;
}

int main() {
    srand(1234567);
    UserData ud;
    sp_data *sp;
    sp_create(&sp);

    sp_diskin_create(&ud.diskin);
    sp_conv_create(&ud.conv);
    sp_ftbl_loadfile(sp, &ud.ft, "imp.wav");

    sp_diskin_init(sp, ud.diskin, "oneart.wav");
    sp_conv_init(sp, ud.conv, ud.ft, 8192);

    sp->len = 44100 * 5;
    sp_process(sp, &ud, process);

    sp_conv_destroy(&ud.conv);
    sp_ftbl_destroy(&ud.ft);
    sp_diskin_destroy(&ud.diskin);

    sp_destroy(&sp);
    return 0;
}
#endif

// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "ExampleBaseApp.h"
#include <cmath>

struct sp_conv;
struct sp_data;
struct sp_ftbl;
typedef float SPFLOAT;
extern "C" int sp_create(sp_data ** spp);
extern "C" int sp_destroy(sp_data ** spp);
extern "C" int sp_conv_compute(sp_data * sp, sp_conv * p, SPFLOAT * in, SPFLOAT * out);
extern "C" int sp_conv_create(sp_conv ** p);
extern "C" int sp_conv_destroy(sp_conv ** p);
extern "C" int sp_conv_init(sp_data * sp, sp_conv * p, sp_ftbl * ft, SPFLOAT iPartLen);
extern "C" int sp_ftbl_destroy(sp_ftbl ** ft);
extern "C" int sp_ftbl_bind(sp_data * sp, sp_ftbl ** ft, SPFLOAT * tbl, size_t size);

class SoundPipeNode : public FunctionNode
{
public:
    SoundPipeNode()
        : FunctionNode(1)
    {
        sp_create(&sp);
        sp_conv_create(&_conv);

        /// @TODO MakeBusFromFile should also do resampling because we need r.sampleRate() for sure
        const bool mixToMono = true;
        impulseResponseClip = MakeBusFromFile("impulse/cardiod-rear-levelled.wav", mixToMono);

        // ft doesn't own the data; it does retain a pointer to it.
        sp_ftbl_bind(sp, &ft, impulseResponseClip->channel(0)->mutableData(), impulseResponseClip->channel(0)->length());
        sp_conv_init(sp, _conv, ft, 8192);

        voiceClip = MakeBusFromFile("samples/voice.ogg", mixToMono);
    }

    virtual ~SoundPipeNode()
    {
        sp_conv_destroy(&_conv);
        sp_ftbl_destroy(&ft);
        sp_destroy(&sp);
    }

    void process(ContextRenderLock & r, int channel, float * pOut, size_t frames)
    {
        float const * data = voiceClip->channel(channel)->data();
        int c = voiceClip->channel(channel)->length() - 1;
        for (int i = 0; i < frames; ++i)
        {
            if (clipFrame >= c)
            {
                *pOut++ = 0.f;
                continue;
            }

            int index = clipFrame + i;
            SPFLOAT in = data[clipFrame + i];
            SPFLOAT out = 0.f;
            sp_conv_compute(sp, _conv, &in, &out);
            *pOut++ = out;
        }
        clipFrame += frames;
    }

    std::shared_ptr<AudioBus> impulseResponseClip;
    std::shared_ptr<AudioBus> voiceClip;
    size_t clipFrame = 0;

    sp_data * sp = nullptr;
    sp_conv * _conv = nullptr;
    sp_ftbl * ft = nullptr;
};

struct SoundPipeApp : public LabSoundExampleApp
{
    virtual void PlayExample(int argc, char ** argv) override
    {
        auto context = lab::Sound::MakeRealtimeAudioContext(lab::Channels::Stereo);

        std::shared_ptr<SoundPipeNode> soundpipeNode;
        std::shared_ptr<GainNode> gainNode;
        {
            ContextRenderLock r(context.get(), "SoundPipe");

            soundpipeNode = std::make_shared<SoundPipeNode>();
            soundpipeNode->setFunction([](ContextRenderLock & r, FunctionNode * me, int channel, float * pOutput, size_t framesToProcess) {
                SoundPipeNode * n = reinterpret_cast<SoundPipeNode *>(me);
                n->process(r, channel, pOutput, framesToProcess);
            });
            soundpipeNode->start(0);

            gainNode = std::make_shared<GainNode>();
            gainNode->gain()->setValue(0.5f);

            context->connect(gainNode, soundpipeNode);
            context->connect(context->destination(), gainNode, 0, 0);
        }

        Wait(std::chrono::seconds(10));
    }
};
