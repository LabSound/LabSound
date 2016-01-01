// License: BSD 3 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioDestination_h
#define AudioDestination_h

namespace lab {

struct AudioIOCallback;

// AudioDestination is an abstraction for audio hardware I/O.
// The audio hardware periodically calls the AudioIOCallback render() method asking it to render/output the next render quantum of audio.
// It optionally will pass in local/live audio input when it calls render().
struct AudioDestination
{
    //@tofix - web audio puts the input initialization on the destination as well. I'm not sure that makes sense.
    static AudioDestination * MakePlatformAudioDestination(AudioIOCallback &, unsigned numberOfOutputChannels, float sampleRate);

    virtual ~AudioDestination() { }

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isPlaying() = 0;

    // Sample-rate conversion may happen in AudioDestination to the hardware sample-rate
    virtual float sampleRate() const = 0;
    static float hardwareSampleRate();

    // maxChannelCount() returns the total number of output channels of the audio hardware.
    // A value of 0 indicates that the number of channels cannot be configured and
    // that only stereo (2-channel) destinations can be created.
    // The numberOfOutputChannels parameter of AudioDestination::create() is allowed to
    // be a value: 1 <= numberOfOutputChannels <= maxChannelCount(),
    // or if maxChannelCount() equals 0, then numberOfOutputChannels must be 2.
    static unsigned long maxChannelCount();
};

} // namespace lab

#endif // AudioDestination_h
