
// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

/// @TODO allow for reversed playback

#include "LabSound/core/SampledAudioNode.h"

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/extended/AudioContextLock.h"
#include "LabSound/extended/Registry.h"

#include "LabSound/extended/VectorMath.h"
#include "internal/Assertions.h"

#include "concurrentqueue/concurrentqueue.h"
#include "libsamplerate/include/samplerate.h"

using namespace lab;

namespace lab {

    /*
    * outputs silence when no source bus is playing
    * OnEnded is dispatched both when the node is stopped, and when the end of a buffer is reached
     */

    struct SRC_Resampler
    {
        SRC_STATE* sampler = nullptr;
        SRC_DATA data;

        SRC_Resampler() = delete;
        SRC_Resampler(SRC_STATE* s) : sampler(s) 
        {
            memset(&data, 0, sizeof(data));
        }

        ~SRC_Resampler()
        {
            if (sampler)
                src_delete(sampler);
        }
    };


    enum SANWorkKind
    {
        SetBus, ClearSchedule, Start
    };
    struct SampledAudioNode::WorkPacket
    {
        SANWorkKind kind;
        double when;          // in context temporal frame
        int32_t grain_start;  // in source bus temporal frame
        int32_t grain_end;
        int32_t cursor;
        int loopCount;        // -1 means forever, 0 means play once, 1 means repeat once -2 is a sentinel value meaning clear the schedule
        std::shared_ptr<AudioBus> sourceBus;
        std::vector<std::shared_ptr<SRC_Resampler>> resampler; // there is one resampler per source channel
    };

    struct SampledAudioNode::Internals
    {
        explicit Internals(AudioContext& ac_)
        : greatest_cursor(-1)
        , ac(ac_.audioContextInterface())
        {
        }
        ~Internals() = default;
        moodycamel::ConcurrentQueue<WorkPacket> incoming;
        std::vector<WorkPacket> scheduled;
        int32_t greatest_cursor = -1;
        std::weak_ptr<AudioContext::AudioContextInterface> ac;
        bool bus_setting_updated = false;
    };

    static AudioParamDescriptor s_saParams[] = {
        {"playbackRate", "RATE",  1.0, 0.0, 1024.},
        {"detune",       "DTUNE", 0.0, 0.0, 1200.},
        {"dopplerRate",  "DPLR",  1.0, 0.0, 1200.}, nullptr};
    static AudioSettingDescriptor s_saSettings[] = {
        {"sourceBus", "SBUS", SettingType::Bus}, nullptr};
    
    AudioNodeDescriptor * SampledAudioNode::desc()
    {
        static AudioNodeDescriptor d = {s_saParams, s_saSettings, 0};
        return &d;
    }

    SampledAudioNode::SampledAudioNode(AudioContext& ac)
        : AudioScheduledSourceNode(ac, *desc())
        , _internals(new Internals(ac))
    {
        // Per WebAudio's AudioBufferSourceNode a SAN will default to silent stereo.
        _self->desiredChannelCount = 2;
        m_sourceBus = setting("sourceBus");
        m_playbackRate = param("playbackRate");
        m_detune = param("detune");
        m_dopplerRate = param("dopplerRate");

        m_sourceBus->setValueChanged([this]() {
            this->_internals->bus_setting_updated = true;
        });

        initialize();
    }

    SampledAudioNode::~SampledAudioNode()
    {
        clearSchedules();
        delete _internals;

        if (isInitialized()) 
            uninitialize();
    }

    void SampledAudioNode::clearSchedules()
    {
        WorkPacket s;
        while (_internals->incoming.try_dequeue(s)) {}
        _internals->incoming.enqueue({ SANWorkKind::ClearSchedule });
    }

    void SampledAudioNode::setBus(std::shared_ptr<AudioBus> sourceBus)
    {
        clearSchedules();
        _internals->incoming.enqueue({ SANWorkKind::SetBus, 0, 0, 0, 0, 0, sourceBus });
        initialize();

        // set the pending pointer, so that a synchronous call to getBus will reflect
        // the value last scheduled. This eliminates a confusing sitatuion where getBus
        // will not be useful until the scheduling queue is serviced
        m_pendingSourceBus = sourceBus;
        setChannelCount(sourceBus->numberOfChannels());
    }

    void SampledAudioNode::start(float when)
    {
        start(when, 0, 0);
    }

    void SampledAudioNode::start(float when, int loopCount)
    {
        start(when, 0, loopCount);
    }

    void SampledAudioNode::start(float when, float grainOffset, int loopCount)
    {
        std::shared_ptr<AudioBus> bus = m_pendingSourceBus;
        if (!bus)
            return;

        auto ac = _internals->ac.lock();
        if (!ac)
            return;

        auto epoch = ac->currentSampleFrame();
        auto rate = ac->sampleRate();
        when = epoch + (int64_t) (when * rate);

        /*
         @TODO
        
        i think _internals->incoming has to go away completely.
        and the SANWork should be attached to the schedule.
        or something. but having two schedulers is confusing.
*/
        _self->scheduler.start(when);

        float r = bus->sampleRate();
        int32_t grainStart = static_cast<uint32_t>(grainOffset * r);
        int32_t grainEnd = bus->length();
        if (grainStart < grainEnd)
        {
            _internals->incoming.enqueue({SANWorkKind::Start,
                                          when,
                                          grainStart, grainEnd, grainStart,
                                          loopCount});
        }
        initialize();
    }

    void SampledAudioNode::start(float when, float grainOffset, float grainDuration, int loopCount)
    {
        std::shared_ptr<AudioBus> bus = m_pendingSourceBus;
        if (!bus)
            return;

        auto ac = _internals->ac.lock();
        if (!ac)
            return;

        auto epoch = ac->currentSampleFrame();
        auto rate = ac->sampleRate();
        when = epoch + (int64_t) (when * rate);

        if (!isPlayingOrScheduled())
            _self->scheduler.start(0.);

        float r = bus->sampleRate();
        int32_t grainStart = static_cast<uint32_t>(grainOffset * r);
        int32_t grainEnd = grainStart + static_cast<uint32_t>(grainDuration * r);
        if (grainEnd > bus->length())
            grainEnd = bus->length() - grainStart;
        if (grainStart < grainEnd)
        {
            _internals->incoming.enqueue({SANWorkKind::Start,
                                          when,
                                          grainStart, grainEnd, grainStart,
                                          loopCount});
        }
        initialize();
    }

    void SampledAudioNode::schedule(float when)
    {
        start(when);
    }

    void SampledAudioNode::schedule(float when, int loopCount)
    {
        start(when, loopCount);
    }

    void SampledAudioNode::schedule(float when, float grainOffset, int loopCount)
    {
        start(when, grainOffset, loopCount);
    }

    void SampledAudioNode::schedule(float when, float grainOffset, float grainDuration, int loopCount)
    {
        start(when, grainOffset, grainDuration, loopCount);
    }

    bool SampledAudioNode::renderSample(ContextRenderLock& r, WorkPacket& schedule,
                                        size_t destinationSampleOffset, size_t frameSize)
    {
        std::shared_ptr<AudioBus> srcBus = m_sourceBus->valueBus();
        AudioBus* dstBus = _self->output.get();
        size_t dstChannelCount = dstBus->numberOfChannels();
        size_t srcChannelCount = srcBus->numberOfChannels();
        ASSERT(dstChannelCount == srcChannelCount);

        float* buffer = dstBus->channel(0)->mutableData();
        float rate = totalPitchRate(r);
        if (fabsf(rate - 1.f) < 1e-3f)
        {
            // no pitch modification
            int write_index = (int) destinationSampleOffset;
            while (write_index < AudioNode::ProcessingSizeInFrames)
            {
                int count = AudioNode::ProcessingSizeInFrames - write_index;
                int remainder = schedule.grain_end - schedule.cursor;
                bool ending = remainder < count;
                count = std::min(count, remainder);

                for (int i = 0; i < srcChannelCount; ++i)
                {
                    float* buffer = dstBus->channel(i)->mutableData();
                    VectorMath::vadd(srcBus->channel(i)->data() + schedule.cursor, 1, 
                                     buffer + write_index, 1,
                                     buffer + write_index, 1, count);
                }

                schedule.cursor += count;
                
                write_index += count;

                if (ending)
                {
                    schedule.cursor = schedule.grain_start; // reset to start

                    if (schedule.loopCount > 0)
                        schedule.loopCount--;
                    else if (schedule.loopCount < 0)
                    {
                        // infinite looping
                    }
                    else
                    {
                        schedule.loopCount = -3;    // signal retirement of the schedule
                        break;                      // and stop the write loop
                    }
                }
            }
        }
        else
        {
            while (schedule.resampler.size() < srcBus->numberOfChannels())
            {
                // samplers are expensive to allocate, so if there's one to recycle, use it
                if (_resamplers.size())
                {
                    auto resampler = _resamplers.back();
                    schedule.resampler.push_back(resampler);
                    _resamplers.pop_back();
                    src_reset(resampler->sampler);
                    memset(&resampler->data, 0, sizeof(resampler->data));
                }
                else
                {
                    int err = 0;
                    schedule.resampler.push_back(std::make_shared<SRC_Resampler>(src_new(SRC_LINEAR, 1, &err)));
                }
            }
            if (schedule.resampler.size() >= srcBus->numberOfChannels())
            {
                // pitch modification
                int write_index = (int) destinationSampleOffset;
                while (write_index < AudioNode::ProcessingSizeInFrames)
                {
                    int count = AudioNode::ProcessingSizeInFrames - write_index;
                    int remainder = schedule.grain_end - schedule.cursor;
                    bool ending = remainder < count;

                    int src_increment = 0;
                    int dst_increment = 0;
                    for (int i = 0; i < srcChannelCount; ++i)
                    {
                        SRC_DATA* src_data = &schedule.resampler[i]->data;
                        float* buffer = dstBus->channel(i)->mutableData();

                        src_data->data_in = srcBus->channel(i)->data() + schedule.cursor;
                        src_data->input_frames = remainder;
                        std::array<float, AudioNode::ProcessingSizeInFrames> buff;
                        src_data->data_out = buff.data();
                        src_data->output_frames = AudioNode::ProcessingSizeInFrames;
                        src_data->src_ratio = 1. / rate;
                        src_data->end_of_input = ending ? 1 : 0;
                        src_process(schedule.resampler[i]->sampler, src_data);
                        VectorMath::vadd(buff.data(), 1, buffer + write_index, 1, buffer + write_index, 1, count);
                        src_increment = static_cast<int>(src_data->input_frames_used);
                        dst_increment = static_cast<int>(src_data->output_frames_gen);
                    }
                    schedule.cursor += src_increment;
                    write_index += dst_increment;

                    ending |= (src_increment == 0) || (dst_increment == 0);

                    if (ending)
                    {
                        if (write_index < AudioNode::ProcessingSizeInFrames) {
                            // if the buffer didn't fully fill the buffer, at the end, zero out the remainder
                            /// @TODO should lerp the end value to zero over a 10ms tail period.
                            /// Going to assume the source data doesn't end with a non-zero value for now.
                            for (int i = 0; i < srcChannelCount; ++i)
                            {
                                SRC_DATA * src_data = &schedule.resampler[i]->data;
                                float* buffer = dstBus->channel(i)->mutableData();
                                for (int j = write_index; j < AudioNode::ProcessingSizeInFrames; ++j)
                                    buffer[j] = 0.f;
                            }
                        }

                        schedule.cursor = schedule.grain_start; // reset to start

                        if (schedule.loopCount > 0)
                            schedule.loopCount--;
                        else if (schedule.loopCount < 0)
                        {
                            // infinite looping, nothing to do
                        }
                        else
                        {
                            schedule.loopCount = -3;    // signal retirement of the schedule
                            break;                      // and stop the write loop
                        }
                    }
                }
            }
        }

        //r.context()->appendDebugBuffer(dstBus, 0, AudioNode::ProcessingSizeInFrames);
        dstBus->clearSilentFlag();
        return true;
    }

    void SampledAudioNode::runWorkBeforeScheduleUpdate(ContextRenderLock& r)
    {
        auto ac = r.context();
        ASSERT(ac); // should never happen
        bool diagnosing_silence = ac->diagnosing().get() == this;

        // manage the introduction of a new source bus.
        //
        // move requested starts to the internal schedule if there's a source bus.
        // if there's no source bus, the schedule requests are discarded.
        auto srcBus = m_sourceBus->valueBus();
        WorkPacket s;
        while (_internals->incoming.try_dequeue(s))
        {
            switch (s.kind) {
                case SANWorkKind::SetBus:
                    m_retainedSourceBus = s.sourceBus;
                    m_sourceBus->setBus(s.sourceBus.get());
                    srcBus = m_retainedSourceBus;
                    srcBus = s.sourceBus;
                    _internals->bus_setting_updated = false; // setting bus causes this -3 state to occur so clear it immediately
                    _self->desiredChannelCount = srcBus->numberOfChannels();
                    if (diagnosing_silence)
                        ac->diagnosed_silence("SampledAudioNode::bus has been set");
                    break;

                case SANWorkKind::ClearSchedule:
                    _internals->scheduled.clear();
                    if (diagnosing_silence)
                        ac->diagnosed_silence("SampledAudioNode::schedule cleared");
                    break;

                case SANWorkKind::Start:
                    if (srcBus)
                    {
                        if (s.when <= 0.0) {
                            // start immediately
                            s.when = 0.0;
                        }
                        else {
                            // start at a future time
                            s.cursor = static_cast<int>(s.when * srcBus->sampleRate());
                            s.when = 0.0;
                        }
                        _internals->scheduled.push_back(s);
                        if (diagnosing_silence)
                            ac->diagnosed_silence("SampledAudioNode::push_back schedule");
                    }
                    else if (diagnosing_silence)
                        ac->diagnosed_silence("SampledAudioNode::schedule encountered, but no source bus has been set");
                    break;
            }
        }
    }

    void SampledAudioNode::process(ContextRenderLock& r, int framesToProcess)
    {
        auto ac = r.context();
        ASSERT(ac); // should never happen
        bool diagnosing_silence = ac->diagnosing().get() == this;

        AudioBus* dstBus = _self->output.get();
        int dstChannelCount = dstBus->numberOfChannels();

        // zero out the buffer for summing, or for silence
        for (int i = 0; i < dstChannelCount; ++i)
            dstBus->channel(i)->zero();

        // if not playing, or the source bus is empty, bail
        auto srcBus = m_sourceBus->valueBus();
        int schedule_count = static_cast<int>(_internals->scheduled.size());
        if (!schedule_count || !srcBus) {
            if (diagnosing_silence)
                ac->diagnosed_silence("SampledAudioNode::process no schedule_count");
            return;
        }

        // compute the frame timing in samples and seconds
        uint64_t quantumStartFrame = r.context()->currentSampleFrame();
        uint64_t quantumEndFrame = quantumStartFrame + AudioNode::ProcessingSizeInFrames;
        double quantumDuration = static_cast<double>(AudioNode::ProcessingSizeInFrames) / r.context()->sampleRate();
        double quantumStartTime = r.context()->currentTime();
        double quantumEndTime = quantumStartTime + quantumDuration;

        // is anything playing in this quantum?
        _internals->greatest_cursor = -1;
        for (int i = 0; i < schedule_count; ++i)
        {
            WorkPacket& s = _internals->scheduled.at(i);
            if (s.when < quantumDuration)   // has s.when counted down to within this quantum?
            {
                int32_t offset = (s.when < quantumStartTime) ? 0 : static_cast<int32_t>(s.when * r.context()->sampleRate());
                renderSample(r, s, (size_t) offset, AudioNode::ProcessingSizeInFrames);
                dstBus->clearSilentFlag();
                if (s.cursor > _internals->greatest_cursor)
                    _internals->greatest_cursor = s.cursor;
            }

            // keep counting the scheduled item down
            s.when -= quantumDuration;
        }

        // retire any schedules whose loopcount is -2, which means that renderSample signalled a finish
        for (int i = 0; i < schedule_count; ++i)
        {
            WorkPacket& s = _internals->scheduled.at(i);
            if (s.loopCount < -1)
            {
                while (s.resampler.size())
                {
                    _resamplers.push_back(s.resampler.back()); // resamplers are expensive to construct, so cache them
                    s.resampler.pop_back();
                }

                if (schedule_count - 1 > i)
                    _internals->scheduled.at(i) = _internals->scheduled.at(schedule_count - 1);
                _internals->scheduled.pop_back();
                --schedule_count;

                _self->scheduler.finish(r);
            }
        }
    }

    int32_t SampledAudioNode::getCursor() const
    {
        return _internals->greatest_cursor;
    }


    /// @TODO change the interface:
    /// // if true is returned, rate_array[0] applies to the entire quantum
    /// // if the computed total rate has any illegal values, true will be returned, and a default rate of 1.f
    /// bool SampledAudioNode::totalPitchRate(ContextRenderLock& r, float*& rate);
    float SampledAudioNode::totalPitchRate(ContextRenderLock& r)
    {
        std::shared_ptr<AudioBus> srcBus = m_sourceBus->valueBus();

        // if there's no bus, pitchrate is defaulted.
        if (!srcBus)
            return 1.f;

        double sampleRateFactor = sampleRateFactor = srcBus->sampleRate() / r.context()->sampleRate();

        /// @fixme these values should be per sample, not per quantum
        /// -or- they should be settings if they don't vary per sample
        double basePitchRate = playbackRate()->value();

        /// @fixme these values should be per sample, not per quantum
        double totalRate = m_dopplerRate->value() * sampleRateFactor * basePitchRate;
        totalRate *= pow(2, detune()->value() / 1200);

        // Sanity check the total rate.  It's very important that the resampler not get any bad rate values.
        totalRate = std::max(0.0, totalRate);

        // Revisit these lower bounds if ever this node can compute ultra low resampling rates.
        if (totalRate < 1.e-2f)
            totalRate = 1;  // a value of zero is considered an illegal value so the default is imposed
        totalRate = std::min(100.0, totalRate);

        bool isTotalRateValid = !std::isnan(totalRate) && !std::isinf(totalRate);
        if (!isTotalRateValid)
            totalRate = 1.0;

        return (float) totalRate;
    }

} // namespace lab
