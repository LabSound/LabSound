/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "LabSoundConfig.h"

#include "AudioFileReaderWin.h"

#include "AudioBus.h"
#include "AudioFileReader.h"
#include "FloatConversion.h"
#include <string>
#include <iostream>

namespace WebCore 
{

AudioFileReader::AudioFileReader(const char* filePath) :  m_data(0), m_dataSize(0) 
{

}

AudioFileReader::AudioFileReader(const void* data, size_t dataSize) :  m_data(data), m_dataSize(dataSize) 
{

}

AudioFileReader::~AudioFileReader() 
{

}

std::unique_ptr<AudioBus> AudioFileReader::createBus(float sampleRate, bool mixToMono) {

	/*
	// m_data
	// m_dataSize

    // Set the sound parameters
    int channelCount = 
    int fsampleRate = 
    int numSamples = 
	int samplesPerChannel = numSamples / channelCount; 

	float *frameData = new float[FileInfos.channels]; 
	float *dataLeft = new float[samplesPerChannel]; 
	float *dataRight = new float[samplesPerChannel]; 

	// Read Frames & Deinterleave if stereo
	for(int count = 0; count < FileInfos.frames; count++) {

		if (channelCount == 2) 
		{
			dataLeft[count] = frameData[0]; 
			dataRight[count] = frameData[1]; 
		}
		else 
		{
			dataLeft[count] = frameData[0]; 
			dataRight[count] = frameData[0]; 
		}

	}

	// Assume stereo...

	// Create Bus before this...

	//memcpy(audioBus->channel(0)->mutableData(), (float*) dataLeft, sizeof(float) * samplesPerChannel);
	//memcpy(audioBus->channel(1)->mutableData(), (float*) dataRight, sizeof(float) * samplesPerChannel);

	// Cleanup

	*/
	std::unique_ptr<AudioBus> audioBus(new AudioBus(2, 0/*samplesPerChannel*/));
    audioBus->setSampleRate(sampleRate); // save for later

	return audioBus;

}

std::unique_ptr<AudioBus> createBusFromAudioFile(const char* filePath, bool mixToMono, float sampleRate) 
{
    AudioFileReader reader(filePath);
    return reader.createBus(sampleRate, mixToMono);
}

std::unique_ptr<AudioBus> createBusFromInMemoryAudioFile(const void* data, size_t dataSize, bool mixToMono, float sampleRate) 
{
    AudioFileReader reader(data, dataSize);
    return reader.createBus(sampleRate, mixToMono); 
}

} // WebCore

