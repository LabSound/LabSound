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

#include <windows.h>
#include <mmsystem.h>
#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioDestinationWin.h"

#include "AudioIOCallback.h"
#include "FloatConversion.h"
#include "VectorMath.h"

namespace WebCore {

// DirectSound needs a larger buffer than 128. This number also needs 
// to be changed in AudioNode.h where ProcessingSizeInFrames = 1024;
const int kBufferSize = 1024;
const float kLowThreshold = -1.0f;
const float kHighThreshold = 1.0f;
    
// Factory method: Windows-implementation
PassOwnPtr<AudioDestination> AudioDestination::create(AudioIOCallback& callback, float sampleRate) {
    return adoptPtr(new AudioDestinationWin(callback, sampleRate));
}

float AudioDestination::hardwareSampleRate() {

	// Default to 44100
	return 44100;

}

AudioDestinationWin::AudioDestinationWin(AudioIOCallback& callback, float sampleRate) :
	m_callback(callback), 
	m_renderBus(2, kBufferSize, false), 
	m_sampleRate(sampleRate), 
	m_isPlaying(false) {

	printf("%f", sampleRate);

	m_renderBus.setSampleRate(hardwareSampleRate());

    configure();

}

AudioDestinationWin::~AudioDestinationWin() {

	if ( dac.isStreamOpen() ) dac.closeStream();

}

void AudioDestinationWin::configure() {

	if ( dac.getDeviceCount() < 1 ) {
		std::cout << "\nNo audio devices found!\n";
	}

	dac.showWarnings(true);

	RtAudio::StreamParameters parameters;
	parameters.deviceId = dac.getDefaultOutputDevice();
	parameters.nChannels = 2;
	parameters.firstChannel = 0;
	unsigned int sampleRate = hardwareSampleRate();

	unsigned int bufferFrames = kBufferSize;

	RtAudio::StreamOptions options;
	options.flags |= RTAUDIO_NONINTERLEAVED;

	try {
		dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &outputCallback, this, &options);
		printf("RTAudio Stream Opened!\n"); 
	} catch (RtError& e) {
		e.printMessage();
	}

}


void AudioDestinationWin::start() {
	
	try {
		dac.startStream();
		m_isPlaying = true;
	} catch (RtError& e) {
		m_isPlaying = false; 
		e.printMessage();
	}

}

void AudioDestinationWin::stop() {

	try {
		dac.stopStream();
		m_isPlaying = false;
	}
	catch (RtError& e) {
		m_isPlaying = true; 
		e.printMessage();
	}
}

// Pulls on our provider to get rendered audio stream.
void AudioDestinationWin::render(int numberOfFrames, void *outputBuffer, void *inputBuffer) {

	float *myOutputBufferOfFloats = (float*) outputBuffer;

	// Tells the given channel to use an externally allocated buffer (rtAudio's)
	 m_renderBus.setChannelMemory(0, myOutputBufferOfFloats, numberOfFrames);
	 m_renderBus.setChannelMemory(1, myOutputBufferOfFloats + (numberOfFrames), numberOfFrames);
	 		
	// Source Bus :: Destination Bus (no source/input) 
	m_callback.render(0, &m_renderBus, numberOfFrames);

	// Clamp values at 0db (i.e., [-1.0, 1.0])
    for (unsigned i = 0; i < m_renderBus.numberOfChannels(); ++i) {
        AudioChannel* channel = m_renderBus.channel(i);
        VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
    }

}

// RTAudio callback ticks and asks for some output... 
int outputCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData ) {
	
	float *fBufOut = (float*) outputBuffer;

	// Buffer is nBufferFrames * channels 
	memset(fBufOut, 0, sizeof(float) * nBufferFrames * 2);

    AudioDestinationWin* audioOutput = static_cast<AudioDestinationWin*>(userData);

	// Get some audio output
    audioOutput->render(nBufferFrames, fBufOut, inputBuffer);

	return 0; 

}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)