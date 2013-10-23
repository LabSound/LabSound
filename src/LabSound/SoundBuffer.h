// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#pragma once

#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "AudioContext.h"

namespace LabSound {

class SoundBuffer
{
public:
    RefPtr<WebCore::AudioBuffer> audioBuffer;
    RefPtr<WebCore::AudioContext> context;
    
    SoundBuffer(RefPtr<WebCore::AudioContext> context, const char* path);
    ~SoundBuffer();
    
    PassRefPtr<WebCore::AudioBufferSourceNode> play(WebCore::AudioNode* outputNode, float when = 0.0f);
    PassRefPtr<WebCore::AudioBufferSourceNode> play(float when = 0.0f);
    
    // This variant starts a sound at a given offset relative to the beginning of the
    // sample, ends it an offfset (relative to the beginning), and optional delays
    // the start. If 0 is passed as end, then the sound will play to the end.
    PassRefPtr<WebCore::AudioBufferSourceNode> play(float start, float end, float when = 0.0f);

    // creates a source node but does not connect it to anything
    PassRefPtr<WebCore::AudioBufferSourceNode> create();
};


} // LabSound
