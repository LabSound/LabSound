#if 0
Coming Soon!

// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#ifndef labsound_granulation_node_h
#define labsound_granulation_node_h

#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioSetting.h"
#include "LabSound/core/AudioScheduledSourceNode.h"
#include "LabSound/core/AudioParam.h"

namespace lab
{
    class GranulationNode : public AudioScheduledSourceNode
    {
        struct grain
        {
            std::shared_ptr<AudioBus> sample;
            std::shared_ptr<AudioBus> window;

            bool in_use {true};

            // Values in samples
            uint64_t grain_start {0};
            uint64_t grain_duration {0};
            uint64_t grain_end {0};
            uint64_t envelope_index {0};

            double playback_frequency;
            double sample_increment;
            double sample_accurate_time; // because of double precision, it's actually subsample-accurate

                                         // position between 0-1, duration in seconds
            grain(std::shared_ptr<AudioBus> _sample, std::shared_ptr<AudioBus> _window, const float sample_rate, 
                const double _position_offset, const double _duration, const double _speed) : sample(_sample), window(_window)
            {
                grain_start = sample->length() * _position_offset;
                grain_duration = _duration * static_cast<double>(sample_rate);
                grain_end = std::min(sample->length(), grain_start + grain_duration);

                playback_frequency = (1.0 / _duration) * _speed;
                sample_accurate_time = (playback_frequency > 0) ? grain_start : grain_end; 
                sample_increment = (playback_frequency != 0.0) ? grain_duration / (sample_rate / playback_frequency) : 0.0;
            }

            void tick(float * out_buffer, const size_t num_frames) 
            {
                const float * windowSamples = window->channel(0)->data();

                for (int i = 0; i < num_frames; ++i)
                {
                    double result = 0.f;

                    if (in_use) 
                    {
                        sample_accurate_time += sample_increment;

                        // Looping behavior
                        if (sample_accurate_time >= grain_end) 
                        {
                            sample_accurate_time = grain_start;
                        }

                        const uint64_t approximate_sample_index = std::floor(sample_accurate_time);
                        const double remainder = sample_accurate_time - approximate_sample_index;

                        uint64_t left = approximate_sample_index;
                        uint64_t right = approximate_sample_index + 1;
                        if (right >= sample->length()) right = 0;

                        // interpolate sample positions (primarily for speed alterations, can be more sophisticated with this later)
                        result = (double) ((1.0 - remainder) * sample->channel(0)->data()[left] + remainder * sample->channel(0)->data()[right]);
                    }

                    envelope_index++;

                    if (envelope_index == grain_duration) 
                    {
                        //in_use = false;
                        envelope_index = 0;
                    }

                    // Apply envelope
                    out_buffer[i] += static_cast<float>(result * windowSamples[envelope_index]); 
                }

            }
        };

        virtual bool propagatesSilence(ContextRenderLock & r) const override;
        virtual double tailTime(ContextRenderLock& r) const override { return 0; }
        virtual double latencyTime(ContextRenderLock& r) const override { return 0; }

        bool RenderGranulation(ContextRenderLock &, AudioBus *, size_t destinationFrameOffset, size_t numberOfFrames);

        std::vector<grain> grain_pool;
        std::shared_ptr<lab::AudioBus> window_bus;

    public:

        GranulationNode();
        virtual ~GranulationNode();

        virtual void process(ContextRenderLock&, size_t framesToProcess) override;
        virtual void reset(ContextRenderLock&) override;

        bool setGrainSource(ContextRenderLock &, std::shared_ptr<AudioBus> sourceBus);
        std::shared_ptr<AudioBus> getGrainSource() const { return grainSourceBus->valueBus(); }

        std::shared_ptr<AudioSetting> grainSourceBus;
        std::shared_ptr<AudioSetting> windowFunc;
        std::shared_ptr<AudioParam> numGrains;
        std::shared_ptr<AudioParam> grainDuration;
        std::shared_ptr<AudioParam> grainPositionMin;
        std::shared_ptr<AudioParam> grainPositionMax;
        std::shared_ptr<AudioParam> grainPlaybackFreq;
    };
    
} // end namespace lab

#endif // end labsound_granulation_node_h

#endif
