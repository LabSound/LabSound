// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#ifndef RECORDER_NODE_H
#define RECORDER_NODE_H

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioContext.h"
#include <mutex>
#include <vector>

namespace lab
{

class RecorderNode : public AudioNode
{
    struct RecorderSettings;
    std::unique_ptr<RecorderSettings> _settings;

    virtual double tailTime(ContextRenderLock & r) const override { return 0; }
    virtual double latencyTime(ContextRenderLock & r) const override { return 0; }
    
public:

    // create a recorder
    RecorderNode(AudioContext & r, int recordChannelCount = 1);

    // create a recorder with a specific configuration
    /// @TODO if the recording cofing has a different sample rate, the output should be resampled.
    /// at the moment the samples get written as is, without rate conversion.
    RecorderNode(AudioContext & r, const AudioStreamConfig & recordingConfig);
    virtual ~RecorderNode();

    static  const char* static_name() { return "Recorder"; }
    virtual const char* name() const override { return static_name(); }
    static AudioNodeDescriptor * desc();

    // AudioNode
    virtual void process(ContextRenderLock &, int bufferSize) override;
    virtual void reset(ContextRenderLock &) override;

    void startRecording();
    void stopRecording();

    float recordedLengthInSeconds() const;

    // create a bus from the recording; it can be used by other nodes
    // such as a SampledAudioNode. The recording is cleared after the bus is
    // created.
    std::unique_ptr<AudioBus> createBusFromRecording(bool mixToMono);

    // returns true for success. The recording is cleared after the bus is created.
    bool writeRecordingToWav(const std::string & filenameWithWavExtension, bool mixToMono);
};

}  // end namespace lab

#endif
