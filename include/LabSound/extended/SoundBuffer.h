// Copyright (c) 2003-2015 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "LabSound/core/AudioBuffer.h"
#include "LabSound/core/AudioBufferSourceNode.h"
#include "LabSound/core/AudioContext.h"

namespace LabSound {

/// @dp rewrite as a node, deprecate audio buffer source node
    
class SoundBuffer {

public:

    std::shared_ptr<WebCore::AudioBuffer> audioBuffer;
    
    SoundBuffer(const char* path, float sampleRate);
    ~SoundBuffer();
    
    // play a sound on the context directly, starting after a certain delay
	std::shared_ptr<WebCore::AudioBufferSourceNode> play(ContextRenderLock&, float when = 0.0f);

    // play a sound on a particular node, starting after a certain delay
    std::shared_ptr<WebCore::AudioBufferSourceNode> play(ContextRenderLock&, std::shared_ptr<WebCore::AudioNode> outputNode, float when = 0.0f);

    // This variant starts a sound at a given offset relative to the beginning of the
    // sample, ends it an offset (relative to the beginning), and optional delays
    // the start. If 0 is passed as end, then the sound will play to the end.
    std::shared_ptr<WebCore::AudioBufferSourceNode> play(ContextRenderLock&, float start, float end, float when = 0.0f);

    // creates a source node sharing the audio buffer but does not connect it to anything
    std::shared_ptr<WebCore::AudioBufferSourceNode> create(ContextRenderLock& r, float sampleRate);
};


} // LabSound
