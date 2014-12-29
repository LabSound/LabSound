// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "AudioContext.h"
#include <WTF/RefPtr.h>

namespace LabSound {

using namespace WTF;

class SoundBuffer {

public:

    std::shared_ptr<WebCore::AudioBuffer> audioBuffer;
    std::weak_ptr<WebCore::AudioContext> context;
    
    SoundBuffer(std::shared_ptr<WebCore::AudioContext>, const char* path);
    ~SoundBuffer();
    
    // play a sound on the context directly, starting after a certain delay
	PassRefPtr<WebCore::AudioBufferSourceNode> play(float when = 0.0f);

    // play a sound on a particular node, starting after a certain delay
    PassRefPtr<WebCore::AudioBufferSourceNode> play(RefPtr<WebCore::AudioNode> outputNode, float when = 0.0f);

    // This variant starts a sound at a given offset relative to the beginning of the
    // sample, ends it an offset (relative to the beginning), and optional delays
    // the start. If 0 is passed as end, then the sound will play to the end.
    PassRefPtr<WebCore::AudioBufferSourceNode> play(float start, float end, float when = 0.0f);

    // creates a source node but does not connect it to anything
    PassRefPtr<WebCore::AudioBufferSourceNode> create();
};


} // LabSound
