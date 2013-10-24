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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioFileReaderWin.h"

#include "AudioBus.h"
#include "AudioFileReader.h"
#include "FloatConversion.h"
#include <string>
#include <iostream>
#include "sndfile.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

AudioFileReader::AudioFileReader(const char* filePath) : 
	m_data(0), 
	m_dataSize(0) {

}

AudioFileReader::AudioFileReader(const void* data, size_t dataSize) : 
	m_data(data),
	m_dataSize(dataSize) {

}

AudioFileReader::~AudioFileReader() {

}

PassOwnPtr<AudioBus> AudioFileReader::createBus(float sampleRate, bool mixToMono) {

	SNDFILE*    myFile;   ///< File descriptor
    MemoryInfos myMemory; ///< Memory read / write data

    // Create AudioBus where we'll put the PCM audio data
    // Define the I/O custom functions for reading from memory
    SF_VIRTUAL_IO VirtualIO;
    VirtualIO.get_filelen = &AudioFileReader::MemoryGetLength;
    VirtualIO.read        = &AudioFileReader::MemoryRead;
    VirtualIO.seek        = &AudioFileReader::MemorySeek;
    VirtualIO.tell        = &AudioFileReader::MemoryTell;
    VirtualIO.write       = &AudioFileReader::MemoryWrite;

    // Initialize the memory data
    myMemory.DataStart = static_cast<const char*>(m_data);
    myMemory.DataPtr   = static_cast<const char*>(m_data);
    myMemory.TotalSize = static_cast<size_t>(m_dataSize);

    // Open the sound file
    SF_INFO FileInfos;

    myFile = sf_open_virtual(&VirtualIO, SFM_READ, &FileInfos, &myMemory);

    if (!myFile) {
		int error = sf_error(myFile); 
		printf(sf_error_number (error)); 
        std::cerr << "Failed to read sound file from memory" << std::endl;
        return false;
    }

    // Set the sound parameters
    int channelCount = FileInfos.channels;
    int fsampleRate = FileInfos.samplerate;
    int numSamples = static_cast<std::size_t>(FileInfos.frames) * channelCount;

	float *audioData = new float[numSamples]; 

	// Unless the end of the file was reached during the read, the return value should equal the number of items requested. 
	// So: Num_items == Num
	int success = sf_read_float(myFile, audioData, numSamples);

	// Assume stereo...
	OwnPtr<AudioBus> audioBus = adoptPtr(new AudioBus(2, numSamples));
    audioBus->setSampleRate(sampleRate); // save for later

	// Mono to Stereo
	if (true) {
		memcpy(audioBus->channel(0)->mutableData(), (float*) audioData, sizeof(float) * numSamples); // Left
		memcpy(audioBus->channel(1)->mutableData(), (float*) audioData, sizeof(float) * numSamples); // Right 
	} else {
		printf("Stereo not supported \n"); 
	}

	sf_close(myFile);

	delete[] audioData;

	// Cleanup
    return audioBus.release();

}

PassOwnPtr<AudioBus> createBusFromAudioFile(const char* filePath, bool mixToMono, float sampleRate) {

    AudioFileReader reader(filePath);
    return reader.createBus(sampleRate, mixToMono);

}

PassOwnPtr<AudioBus> createBusFromInMemoryAudioFile(const void* data, size_t dataSize, bool mixToMono, float sampleRate) {

    AudioFileReader reader(data, dataSize);
    return reader.createBus(sampleRate, mixToMono); 

}


/////////////////////////////////////////////////////////////////////////////////////
/// Functions for implementing custom read and write to memory files
/////////////////////////////////////////////////////////////////////////////////////
sf_count_t AudioFileReader::MemoryGetLength(void* UserData) {

    MemoryInfos* Memory = static_cast<MemoryInfos*>(UserData);
    return Memory->TotalSize;
}

sf_count_t AudioFileReader::MemoryRead(void* Ptr, sf_count_t Count, void* UserData) {

    MemoryInfos* Memory = static_cast<MemoryInfos*>(UserData);

    sf_count_t Position = Memory->DataPtr - Memory->DataStart;
    if (Position + Count >= Memory->TotalSize)
        Count = Memory->TotalSize - Position;

    memcpy(Ptr, Memory->DataPtr, static_cast<std::size_t>(Count));

    Memory->DataPtr += Count;

    return Count;
}

sf_count_t AudioFileReader::MemorySeek(sf_count_t Offset, int Whence, void* UserData) {

    MemoryInfos* Memory = static_cast<MemoryInfos*>(UserData);

    sf_count_t Position = 0;
    switch (Whence)
    {
        case SEEK_SET :
            Position = Offset;
            break;
        case SEEK_CUR :
            Position = Memory->DataPtr - Memory->DataStart + Offset;
            break;
        case SEEK_END :
            Position = Memory->TotalSize - Offset;
            break;
        default :
            Position = 0;
            break;
    }

    if (Position >= Memory->TotalSize)
        Position = Memory->TotalSize - 1;
    else if (Position < 0)
        Position = 0;

    Memory->DataPtr = Memory->DataStart + Position;

    return Position;

}

sf_count_t AudioFileReader::MemoryTell(void* UserData) {

    MemoryInfos* Memory = static_cast<MemoryInfos*>(UserData);
    return Memory->DataPtr - Memory->DataStart;
}

sf_count_t AudioFileReader::MemoryWrite(const void*, sf_count_t, void*) {
    return 0;
}

} // WebCore

#endif // ENABLE(WEB_AUDIO)

