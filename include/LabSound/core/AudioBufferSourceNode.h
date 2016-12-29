// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioBufferSourceNode_h
#define AudioBufferSourceNode_h

#include "LabSound/core/AudioBuffer.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/PannerNode.h"

#include <memory>

namespace lab {

class AudioContext;
class AudioBus;

// AudioBufferSourceNode is an AudioNode representing an audio source from an in-memory audio asset represented by an AudioBuffer.
// It generally will be used for short sounds which require a high degree of scheduling flexibility (can playback in rhythmically perfect ways).

class AudioBufferSourceNode : public AudioScheduledSourceNode {
public:
    AudioBufferSourceNode(float sampleRate);
    virtual ~AudioBufferSourceNode();

    // AudioNode
    virtual void process(ContextRenderLock&, size_t framesToProcess) override;
    virtual void reset(ContextRenderLock&) override;

    // setBuffer() is called on the main thread.  This is the buffer we use for playback.
    // returns true on success.
    bool setBuffer(ContextRenderLock&, std::shared_ptr<AudioBuffer>);

    std::shared_ptr<AudioBuffer> buffer() { return m_buffer; }

    // numberOfChannels() returns the number of output channels.  This value equals the number of channels from the buffer.
    // If a new buffer is set with a different number of channels, then this value will dynamically change.
    unsigned numberOfChannels(ContextRenderLock&);

    // Play-state
    void startGrain(double when, double grainOffset);
    void startGrain(double when, double grainOffset, double grainDuration);

    // Note: the attribute was originally exposed as .looping, but to be more consistent in naming with <audio>
    // and with how it's described in the specification, the proper attribute name is .loop
    // The old attribute is kept for backwards compatibility.
    bool loop() const { return m_isLooping; }
    void setLoop(bool looping) { m_isLooping = looping; }

    // Loop times in seconds.
    double loopStart() const { return m_loopStart; }
    double loopEnd() const { return m_loopEnd; }
    void setLoopStart(double loopStart) { m_loopStart = loopStart; }
    void setLoopEnd(double loopEnd) { m_loopEnd = loopEnd; }

    // Deprecated.
    bool looping();
    void setLooping(bool);

    std::shared_ptr<AudioParam> gain() { return m_gain; }
    std::shared_ptr<AudioParam> playbackRate() { return m_playbackRate; }

    // If a panner node is set, then we can incorporate doppler shift into the playback pitch rate.
    void setPannerNode(PannerNode*);
    virtual void clearPannerNode() override;

    // If we are no longer playing, propagate silence ahead to downstream nodes.
    virtual bool propagatesSilence(double now) const override;

private:
    virtual double tailTime() const override { return 0; }
    virtual double latencyTime() const override { return 0; }

    // Returns true on success.
    bool renderFromBuffer(ContextRenderLock&, AudioBus*, unsigned destinationFrameOffset, size_t numberOfFrames);

    // Render silence starting from "index" frame in AudioBus.
    bool renderSilenceAndFinishIfNotLooping(ContextRenderLock& r, AudioBus*, unsigned index, size_t framesToProcess);

    // m_buffer holds the sample data which this node outputs.
    std::shared_ptr<AudioBuffer> m_buffer;

    // Pointers for the buffer and destination.
    std::unique_ptr<const float*[]> m_sourceChannels;
    std::unique_ptr<float*[]> m_destinationChannels;

    // Used for the "gain" and "playbackRate" attributes.
    std::shared_ptr<AudioParam> m_gain;
    std::shared_ptr<AudioParam> m_playbackRate;

    // If m_isLooping is false, then this node will be done playing and become inactive after it reaches the end of the sample data in the buffer.
    // If true, it will wrap around to the start of the buffer each time it reaches the end.
    bool m_isLooping;

    bool m_startRequested;
    double m_requestWhen;
    double m_requestGrainOffset;
    double m_requestGrainDuration;

    double m_loopStart;
    double m_loopEnd;

    // m_virtualReadIndex is a sample-frame index into our buffer representing the current playback position.
    // Since it's floating-point, it has sub-sample accuracy.
    double m_virtualReadIndex;

    // Granular playback
    bool m_isGrain;
    double m_grainOffset; // in seconds
    double m_grainDuration; // in seconds

    // totalPitchRate() returns the instantaneous pitch rate (non-time preserving).
    // It incorporates the base pitch rate, any sample-rate conversion factor from the buffer, and any doppler shift from an associated panner node.
    double totalPitchRate(ContextRenderLock&);

    // m_lastGain provides continuity when we dynamically adjust the gain.
    float m_lastGain;

    // We optionally keep track of a panner node which has a doppler shift that is incorporated into
    // the pitch rate. We manually manage ref-counting because we want to use RefTypeConnection.
    PannerNode* m_pannerNode;
};

} // namespace lab

#endif // AudioBufferSourceNode_h
