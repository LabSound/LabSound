// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioDevice.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioContext.h"
#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioParam.h"
#include "LabSound/core/AudioSourceProvider.h"
#include "LabSound/extended/AudioContextLock.h"

#include "internal/Assertions.h"
#include "internal/DenormalDisabler.h"

// Non platform-specific helper functions

using namespace lab;


AudioNodeDescriptor * AudioDestinationNode::desc()
{
    static AudioNodeDescriptor d {nullptr, nullptr};
    return &d;
}

namespace lab {

AudioDestinationNode::AudioDestinationNode(
    AudioContext& ac,
    std::shared_ptr<AudioDevice> device)
: AudioNode(ac, *desc())
, _context(&ac)
, _platformAudioDevice(device)
{
    // Node-specific default mixing rules.
    //_self->m_channelCountMode = ChannelCountMode::Explicit;
    _self->m_channelInterpretation = ChannelInterpretation::Speakers;

    int numChannels = _platformAudioDevice->getOutputConfig().desired_channels;
    ContextGraphLock glock(&ac, "AudioDestinationNode");
    AudioNode::setChannelCount(numChannels);

    initialize();
}

/*
 The most elementary process is to map/sum inputs to output channels
 */
void AudioNode::process(ContextRenderLock &, int bufferSize)
{
    switch (_self->inputs.size()) {
        case 0: return;
        case 1: _self->output->copyFrom(_self->inputs[0].node->_self->output);
            return;
        default:
            break;
    }

    // zero the output
    int c = _self->output->numberOfChannels();
    for (int i = 0; i < c; ++i)
        _self->output->channel(i)->zero();
    
    // sum all the inputs into their mapped output channels
    for (auto &i : _self->inputs) {
        auto dst = _self->output->channel(i.outputChannel);
        auto src = i.node->_self->output->channel(i.inputChannel);
        if (dst && src)
            dst->sumFrom(src);
    }
}

//static
void AudioNode::gatherInputsAndUpdateSchedules(ContextRenderLock& r, 
                             AudioNode* root,
                                   std::vector<AudioNode*>& n,
                                   std::vector<AudioNode*>& un,
                                   int frames,
                                   uint64_t current_sample_frame)
{
    for (auto& p : root->_self->params) {
        if (p->overridingInput()) {
            auto input = p->overridingInput();
            if (input->_self->graphEpoch <= current_sample_frame) {
                input->_self->graphEpoch = current_sample_frame + ProcessingSizeInFrames;
                bool playing = input->updateSchedule(r, frames);
                if (playing) {
                    n.push_back(input.get());
                    gatherInputsAndUpdateSchedules(r, input.get(), n, un, frames, current_sample_frame);
                }
                else
                    un.push_back(input.get());
            }
        }
    }

    for (auto& i : root->_self->inputs) {
        if (i.node->_self->graphEpoch <= current_sample_frame) {
            i.node->_self->graphEpoch = current_sample_frame + ProcessingSizeInFrames;
            bool playing = i.node->updateSchedule(r, frames);
            if (playing) {
                n.push_back(i.node.get());
                gatherInputsAndUpdateSchedules(r, i.node.get(), n, un, frames, current_sample_frame);
            }
            else
                un.push_back(i.node.get());
        }
    }
}

void AudioNode::render(AudioContext* context,
                       AudioSourceProvider* provider,
        AudioBus* optional_hardware_input, 
        AudioBus* write_graph_output_data_here)
{
    ProfileScope selfProfile(_self->totalTime);
    ProfileScope profile(_self->graphTime);
    
    auto currentSampleFrame = context->audioContextInterface().lock()->currentSampleFrame();

    // The audio system might still be invoking callbacks during shutdown,
    // so bail out if called without a context
    if (!context || !context->isInitialized() || !write_graph_output_data_here)
        return;

    // bail if shutting down.
    auto ac = context->audioContextInterface().lock();
    if (!ac)
        return;

    ContextRenderLock renderLock(context, "lab::pull_graph");
    if (!renderLock.context())
        return;  // return if couldn't acquire lock

    // Denormals can slow down audio processing.
    // Use an RAII object to protect all AudioNodes processed within this scope.

    /// @TODO under what circumstance do they arise?
    /// If they come from input data such as loaded WAV files, they should be corrected
    /// at source. If they can result from signal processing; again, where? The
    /// signal processing should not produce denormalized values.

    DenormalDisabler denormalDisabler;

    // Let the context take care of any business at the start of each render quantum.
    context->handlePreRenderTasks(renderLock);

    if (_self->desiredChannelCount != _self->output->numberOfChannels()) {
        _self->output->setNumberOfChannels(renderLock, _self->desiredChannelCount);
    }
    
    // Prepare the local audio input provider for this render quantum.
    if (provider)
    {
    //    provider->fetchSamples(fill_source_input_data_here);
    }

    int frames = write_graph_output_data_here->length();
    
    std::vector<AudioNode*> processSchedule;
    std::vector<AudioNode*> unscheduled;
    gatherInputsAndUpdateSchedules(renderLock, this, processSchedule, unscheduled, frames, currentSampleFrame);
    
    auto diagnosing = context->diagnosing();
    for (auto n : unscheduled) {
        // unscheduled nodes may be in the graph, so their channel counts must be updated
        if (n->_self->desiredChannelCount != n->_self->output->numberOfChannels()) {
            n->_self->output->setNumberOfChannels(renderLock, n->_self->desiredChannelCount);
        }
    }
    for (auto n : processSchedule) {
        // update output channel counts
        if (n->_self->desiredChannelCount != n->_self->output->numberOfChannels()) {
            n->_self->output->setNumberOfChannels(renderLock, n->_self->desiredChannelCount);
        }
        
        bool diagnosing_silence = n == diagnosing.get();
                
        // there may need to be silence at the beginning or end of the current quantum.
        
        int start_zero_count = n->_self->scheduler._renderOffset;
        int final_zero_start = n->_self->scheduler._renderOffset + n->_self->scheduler._renderLength;
        int final_zero_count = frames - final_zero_start;
        
        //  initialize the busses with start and final zeroes.
        if (start_zero_count)
        {
            for (int i = 0; i < n->_self->output->numberOfChannels(); ++i)
                memset(n->_self->output->channel(i)->mutableData(), 0, sizeof(float) * start_zero_count);
        }
        
        if (final_zero_count)
        {
            for (int i = 0; i < n->_self->output->numberOfChannels(); ++i)
                memset(n->_self->output->channel(i)->mutableData() + final_zero_start, 0, sizeof(float) * final_zero_count);
        }

        // do the signal processing

        n->process(renderLock, frames);

        // clean pops resulting from starting or stopping

        const int start_envelope = ProcessingSizeInFrames / 2;
        const int end_envelope = ProcessingSizeInFrames / 2;
        
        #define OOS(x) (float(x) / float(steps))
        if (start_zero_count > 0 || n->_self->scheduler._playbackState == SchedulingState::FADE_IN)
        {
            int steps = start_envelope;
            int damp_start = start_zero_count;
            int damp_end = start_zero_count + steps;
            if (damp_end > frames)
            {
                damp_end = frames;
                steps = frames - start_zero_count;
            }

            // damp out the first samples
            if (damp_end > 0)
                for (int i = 0; i < n->_self->output->numberOfChannels(); ++i)
                {
                    float* data = n->_self->output->channel(i)->mutableData();
                    for (int j = damp_start; j < damp_end; ++j)
                        data[j] *= OOS(j - damp_start);
                }
        }

        if (final_zero_count > 0 || n->_self->scheduler._playbackState == SchedulingState::STOPPING)
        {
            int steps = end_envelope;
            int damp_end, damp_start;
            if (!final_zero_count)
            {
                damp_end = frames - final_zero_count;
                damp_start = damp_end - steps;
            }
            else
            {
                damp_end = frames - final_zero_count;
                damp_start = damp_end - steps;
            }

            if (damp_start < 0)
            {
                damp_start = 0;
                steps = damp_end;
            }

            // damp out the last samples
            if (steps > 0)
            {
                //printf("out: %d %d\n", damp_start, damp_end);
                for (int i = 0; i < n->_self->output->numberOfChannels(); ++i)
                {
                    float* data = n->_self->output->channel(i)->mutableData();
                    for (int j = damp_start; j < damp_end; ++j)
                        data[j] *= OOS(damp_end - j);
                }
            }
        }

        n->unsilenceOutputs(renderLock);
    }
    
    process(renderLock, frames);

    write_graph_output_data_here->copyFrom(_self->output);

    // Process nodes which need extra help because they are not connected to anything,
    // but still have work to do
    context->processAutomaticPullNodes(renderLock, frames);

    // Let the context take care of any business at the end of each render quantum.
    context->handlePostRenderTasks(renderLock);

    profile.finalize();
    selfProfile.finalize();
}

#if 0
void AudioDestinationNode::offlineRender(AudioBus* dst, size_t framesToProcess)
{
    static const int offlineRenderSizeQuantum = AudioNode::ProcessingSizeInFrames;

    if (!dst || !framesToProcess || !_context || !_context->isInitialized())
        return;

    bool isRenderBusAllocated = dst->length() >= offlineRenderSizeQuantum;
    ASSERT(isRenderBusAllocated);
    if (!isRenderBusAllocated)
        return;

    LOG_TRACE("offline rendering started");

    while (framesToProcess > 0) {
        _context->update();
        AudioSourceProvider* asp = nullptr;
        render(_context, asp, nullptr, dst, _last_info);

        // Update sampling info
        const int index = 1 - (_last_info.current_sample_frame & 1);
        const uint64_t t = _last_info.current_sample_frame & ~1;
        _last_info.current_sample_frame = t + offlineRenderSizeQuantum + index;
        _last_info.current_time = _last_info.current_sample_frame / static_cast<double>(_last_info.sampling_rate);
        _last_info.epoch[index] += std::chrono::nanoseconds {
                static_cast<uint64_t>(1.e9 * (double) framesToProcess / (double) offlineRenderSizeQuantum)};

        framesToProcess--;
    }
}
#endif

void AudioDestinationNode::initialize()
{
    if (!isInitialized())
        AudioNode::initialize();
}

void AudioDestinationNode::uninitialize()
{
    if (!isInitialized())
        return;
    
    _platformAudioDevice->stop();
    AudioNode::uninitialize();
    _platformAudioDevice.reset();
}

void AudioDestinationNode::reset(ContextRenderLock &)
{
    _platformAudioDevice->stop();
    _platformAudioDevice->start();
}

} // lab
