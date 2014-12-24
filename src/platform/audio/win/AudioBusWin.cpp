/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#include "LabSoundConfig.h"
#include "libsndfile\sndfile.h"

#include "AudioBus.h"
#include "AudioFileReader.h"
#include "AutodrainedPool.h"

#include "SoundBuffer.h"
#include <direct.h>
#include <iostream>

namespace WebCore {

	std::unique_ptr<AudioBus> AudioBus::loadPlatformResource(const char *name, float sampleRate) {

		char cwd[MAX_PATH];

		_getcwd(cwd, MAX_PATH); 

		std::string pathToFile( std::string(cwd) + std::string("\\resources\\") + std::string(name) + std::string(".wav"));

		FILE* f = fopen(pathToFile.c_str(), "rb");

		std::cout << pathToFile << std::endl; 

        if (f) {

            fseek(f, 0, SEEK_END);
            int l = ftell(f);
            fseek(f, 0, SEEK_SET);
            unsigned char* data = new unsigned char[l];
            fread(data, 1, l, f);
            fclose(f);
            
            bool mixToMono = false;

            PassRefPtr<ArrayBuffer> fileDataBuffer = ArrayBuffer::create(reinterpret_cast<float*>(data), l);
            delete [] data;

			return std::unique_ptr<AudioBus>(createBusFromInMemoryAudioFile(fileDataBuffer->data(), fileDataBuffer->byteLength(), false, 44100));
        }

		ASSERT_NOT_REACHED();

		return nullptr;

	}


} // namespace WebCore

