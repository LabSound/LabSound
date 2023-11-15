// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef LABSOUND_EXAMPLE_BASE_APP_H
#define LABSOUND_EXAMPLE_BASE_APP_H

#include "LabSound/LabSound.h"
#include "LabSound/backends/AudioDevice_RtAudio.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace lab;

// Returns input, output
inline std::pair<AudioStreamConfig, AudioStreamConfig>
    GetDefaultAudioDeviceConfiguration(const bool with_input = true)
{    
    const std::vector<AudioDeviceInfo> audioDevices = lab::AudioDevice_RtAudio::MakeAudioDeviceList();
    AudioDeviceInfo defaultOutputInfo, defaultInputInfo;
    for (auto & info : audioDevices) {
        if (info.is_default_output)
            defaultOutputInfo = info;
        if (info.is_default_input)
            defaultInputInfo = info;
    }
    
    AudioStreamConfig outputConfig;
    if (defaultOutputInfo.index != -1) {
        outputConfig.device_index = defaultOutputInfo.index;
        outputConfig.desired_channels = std::min(uint32_t(2), defaultOutputInfo.num_output_channels);
        outputConfig.desired_samplerate = defaultOutputInfo.nominal_samplerate;
    }
    
    AudioStreamConfig inputConfig;
    if (with_input) {
        if (defaultInputInfo.index != -1) {
            inputConfig.device_index = defaultInputInfo.index;
            inputConfig.desired_channels = std::min(uint32_t(1), defaultInputInfo.num_input_channels);
            inputConfig.desired_samplerate = defaultInputInfo.nominal_samplerate;
        }
        else {
            throw std::invalid_argument("the default audio input device was requested but none were found");
        }
    }
    
    // RtAudio doesn't support mismatched input and output rates.
    // this may be a pecularity of RtAudio, but for now, force an RtAudio
    // compatible configuration
    if (defaultOutputInfo.nominal_samplerate != defaultInputInfo.nominal_samplerate) {
        float min_rate = std::min(defaultOutputInfo.nominal_samplerate, defaultInputInfo.nominal_samplerate);
        inputConfig.desired_samplerate = min_rate;
        outputConfig.desired_samplerate = min_rate;
        printf("Warning ~ input and output sample rates don't match, attempting to set minimum\n");
    }
    return {inputConfig, outputConfig};
}

struct labsound_example
{
    std::mt19937 randomgenerator;

    std::vector<std::shared_ptr<lab::AudioNode>> _nodes;
    std::vector<std::shared_ptr<lab::PowerMonitorNode>> _powerNodes;
    std::shared_ptr<lab::AudioContext> _context;
    std::shared_ptr<lab::RecorderNode> _recorder;

    labsound_example(std::shared_ptr<lab::AudioContext> context, bool with_input)
    : _context(context)
    {
    }
    virtual ~labsound_example() = default;    

    virtual void play(int argc, char** argv) = 0;

    void AddRecorder(std::shared_ptr<lab::AudioNode> node)
    {
        _recorder.reset(new RecorderNode(*_context.get(), _context->destinationNode()->device()->getOutputConfig()));
        _context->addAutomaticPullNode(_recorder);
        _context->connect(_recorder, node, 0, 0);
        _recorder->startRecording();
    }
   
   void StopRecording(const std::string& wav_out_path)
    {
       _recorder->stopRecording();
       _context->removeAutomaticPullNode(_recorder);
       _recorder->writeRecordingToWav(wav_out_path.c_str(), false);
   }

    float MidiToFrequency(int midiNote)
    {
        return 440.0f * pow(2.0f, (midiNote - 57.0f) / 12.0f);
    }

    void Wait(uint32_t ms)
    {
        int32_t remainder = static_cast<int32_t>(ms);
        while (remainder > 200) {
            remainder -= 200;
            std::chrono::milliseconds t(200);
            std::this_thread::sleep_for(t);
            if (_powerNodes.size()) {
                for (auto& n : _powerNodes) {
                    printf("%f ", n->db());
                }
                printf("\n");
            }
        }
        if (remainder < 200 && remainder > 0) {
            std::chrono::milliseconds t(remainder);
            std::this_thread::sleep_for(t);
        }
    }
    
    void AddMonitorNodes() {
        for (auto n : _nodes) {
            std::shared_ptr<lab::PowerMonitorNode> pn(new PowerMonitorNode(*_context.get()));
            _context->connect(pn, n, 0, 0);
            _context->addAutomaticPullNode(pn);
            _powerNodes.push_back(pn);
        }
    }
    
    inline std::vector<std::string> SplitCommandLine(int argc, char ** argv)
    {
        // takes a string, and separates out according to embedded quoted strings
        // the quotes are preserved, and quotes are escaped.
        // examples
        // * abc > abc
        // * abc "def" > abc, "def"
        // * a "def" ghi > a, "def", ghi
        // * a\"bc > a\"bc

        auto Separate = [](const std::string & input) -> std::vector<std::string>
        {
            std::vector<std::string> output;

            size_t curr = 0;
            size_t start = 0;
            size_t end = input.length();
            bool inQuotes = false;

            while (curr < end)
            {
                if (input[curr] == '\\')
                {
                    ++curr;
                    if (curr != end && input[curr] == '\"')
                        ++curr;
                }
                else
                {
                    if (input[curr] == '\"')
                    {
                        // no empty string if not in quotes, otherwise preserve it
                        if (inQuotes || (start != curr))
                        {
                            output.push_back(input.substr(start - (inQuotes ? 1 : 0), curr - start + (inQuotes ? 2 : 0)));
                        }
                        inQuotes = !inQuotes;
                        start = curr + 1;
                    }
                    ++curr;
                }
            }

            // catch the case of a trailing substring that was not quoted, or a completely unquoted string
            if (curr - start > 0) output.push_back(input.substr(start, curr - start));

            return output;
        };

        // join the command line together so quoted strings can be found
        std::string cmd;
        for (int i = 1; i < argc; ++i)
        {
            if (i > 1) cmd += " ";
            cmd += std::string(argv[i]);
        }

        // separate the command line, respecting quoted strings
        std::vector<std::string> result = Separate(cmd);
        result.insert(result.begin(), std::string{argv[0]});
        return result;
    }

    inline std::shared_ptr<AudioBus> MakeBusFromSampleFile(char const*const name, int argc, char** argv)
    {
        std::string path_prefix;
        auto cmds = SplitCommandLine(argc, argv);

        if (cmds.size() > 1) 
            path_prefix = cmds[1] + "/";  // cmds[0] is the path to the exe

        std::string path = path_prefix + name;
        std::shared_ptr<AudioBus> bus = MakeBusFromFile(path, false);

        if (!bus) {
            path = std::string(SAMPLE_SRC_DIR) + "/" + std::string(name);
            bus = MakeBusFromFile(path, false);
        }

        if (!bus) 
            throw std::runtime_error("couldn't open " + path);

        return bus;
    }
};

#endif
