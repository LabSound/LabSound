// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "SoundBuffer.h"

#include <stdio.h>
#include <iostream>

namespace LabSound {
    
    using namespace WebCore;
    
    SoundBuffer::SoundBuffer(RefPtr<AudioContext> context, const char* path)
    : context(context)
    {
        FILE* f = fopen(path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            int l = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t* data = new uint8_t[l];
            fread(data, 1, l, f);
            fclose(f);
            
            ExceptionCode ec;
            bool mixToMono = true;
            PassRefPtr<ArrayBuffer> fileDataBuffer = ArrayBuffer::create(data, l);
            delete [] data;
            
            // create an audio buffer from the file data. The file data will be
            // parsed, and does not need to be retained.
            audioBuffer = context->createBuffer(fileDataBuffer.get(), mixToMono, ec);
        }
        else
            std::cerr << "File not found " << path << std::endl;
    }
    
    SoundBuffer::~SoundBuffer()
    {
    }
    
    PassRefPtr<AudioBufferSourceNode> SoundBuffer::create()
    {
        if (audioBuffer) {
            RefPtr<AudioBufferSourceNode> sourceBuffer;
            sourceBuffer = context->createBufferSource();
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(audioBuffer.get());
            return sourceBuffer;
        }
        return 0;
    }
    
    PassRefPtr<AudioBufferSourceNode> SoundBuffer::play(AudioNode* outputNode, float when)
    {
        if (audioBuffer) {
            RefPtr<AudioBufferSourceNode> sourceBuffer;
            sourceBuffer = context->createBufferSource();
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(audioBuffer.get());
            
            // bus the sound to the mixer.
            ExceptionCode ec;
            sourceBuffer->connect(outputNode, 0, 0, ec);
            sourceBuffer->start(when);
            return sourceBuffer;
        }
        return 0;
    }
    
    PassRefPtr<AudioBufferSourceNode> SoundBuffer::play(float when)
    {
        if (audioBuffer) {
            return play(context->destination(), when);
        }
        return 0;
    }
    
    // This variant starts a sound at a given offset relative to the beginning of the
    // sample, ends it an offfset (relative to the beginning), and optional delays
    // the start. If 0 is passed as end, then the sound will play to the end.
    PassRefPtr<AudioBufferSourceNode> SoundBuffer::play(float start, float end, float when)
    {
        if (audioBuffer) {
            if (end == 0)
                end = audioBuffer->duration();
            
            RefPtr<AudioBufferSourceNode> sourceBuffer;
            sourceBuffer = context->createBufferSource();
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(audioBuffer.get());
            
            // bus the sound to the mixer.
            ExceptionCode ec;
            sourceBuffer->connect(context->destination(), 0, 0, ec);
            sourceBuffer->startGrain(when, start, end - start);
            return sourceBuffer;
        }
        return 0;
    }

} // LabSound
