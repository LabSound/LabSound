// Copyright (c) 2003-2013 Nick Porcino, All rights reserved.
// License is MIT: http://opensource.org/licenses/MIT

#include "SoundBuffer.h"
#include "AudioContextLock.h"
#include "AudioDestinationNode.h"
#include <stdio.h>
#include <iostream>

namespace LabSound {
    
    using namespace WebCore;
    
    SoundBuffer::SoundBuffer(const char* path, float sampleRate)
    {
        FILE* f = fopen(path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            int l = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t* data = new uint8_t[l];
            fread(data, 1, l, f);
            fclose(f);
            
            bool mixToMono = true;
            
            // create an audio buffer from the file data. The file data will be
            // parsed, and does not need to be retained.
            audioBuffer = AudioBuffer::createFromAudioFileData(data, l, mixToMono, sampleRate);
            delete [] data;
        }
        else
            std::cerr << "File not found " << path << std::endl;
    }
    
    SoundBuffer::~SoundBuffer()
    {
    }
  
    std::shared_ptr<AudioBufferSourceNode> SoundBuffer::create(ContextRenderLock& r, float sampleRate)
    {
        if (audioBuffer) {
            std::shared_ptr<AudioBufferSourceNode> sourceBuffer(new AudioBufferSourceNode(sampleRate));
            
            // Connect the source node to the parsed audio data for playback
            sourceBuffer->setBuffer(r, audioBuffer);
            return sourceBuffer;
        }
        return nullptr;
    }
    
	// Output to the default context output 
	std::shared_ptr<AudioBufferSourceNode> SoundBuffer::play(ContextRenderLock& r, float when)
	{
		if (audioBuffer && r.context()) {
			return play(r, r.context()->destination(), when);
		}
        return nullptr;
	}

	// Output to a specific note 
    std::shared_ptr<AudioBufferSourceNode> SoundBuffer::play(ContextRenderLock& r,
                                                             std::shared_ptr<AudioNode> outputNode, float when)
    {
        auto ac = r.context();
        if (audioBuffer && ac) {
            std::shared_ptr<AudioBufferSourceNode> sourceBufferNode(new AudioBufferSourceNode(outputNode->sampleRate()));
            sourceBufferNode->setBuffer(r, audioBuffer);
            
            // bus the sound to the output node
            ac->connect(sourceBufferNode, outputNode);
            sourceBufferNode->start(when);
            ac->holdSourceNodeUntilFinished(sourceBufferNode);
            return sourceBufferNode;
        }
        return nullptr;
    }
    
    // This variant starts a sound at a given offset relative to the beginning of the
    // sample, ends it an offfset (relative to the beginning), and optional delays
    // the start. If 0 is passed as end, then the sound will play to the end.
    std::shared_ptr<AudioBufferSourceNode> SoundBuffer::play(ContextRenderLock& r, float start, float end, float when)
    {
        if (audioBuffer) {

            if (end == 0)
                end = audioBuffer->duration();
            
            auto ac = r.context();
            if (!ac)
                return nullptr;
            
            std::shared_ptr<AudioBufferSourceNode> sourceBufferNode(new AudioBufferSourceNode(ac->destination()->sampleRate()));
            
            // Connect the source node to the parsed audio data for playback
            sourceBufferNode->setBuffer(r, audioBuffer);
            
            // bus the sound to the mixer.
            ExceptionCode ec;
            sourceBufferNode->connect(r.context(), ac->destination().get(), 0, 0, ec);
            sourceBufferNode->startGrain(when, start, end - start);
            r.context()->holdSourceNodeUntilFinished(sourceBufferNode);

            return sourceBufferNode;
        }

        return nullptr;

    }

} // LabSound
