// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#pragma once

#ifndef LABSOUND_EXAMPLE_BASE_APP_H
#define LABSOUND_EXAMPLE_BASE_APP_H

#include "LabSound/LabSound.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <string>

// In the future, this class could do all kinds of clever things, like setting up the context,
// handling recording functionality, etc.

#include <string>
#include <vector>

using namespace lab;

#ifdef _MSC_VER
#include <windows.h>
std::string PrintCurrentDirectory()
{
    char buffer[MAX_PATH] = {0};
    GetCurrentDirectory(MAX_PATH, buffer);
    return std::string(buffer);
}
#endif

struct labsound_example
{
    std::mt19937 randomgenerator;

    std::vector<std::shared_ptr<lab::AudioNode>> _nodes;

    virtual void play(int argc, char** argv) = 0;

    float MidiToFrequency(int midiNote)
    {
        return 440.0f * pow(2.0f, (midiNote - 57.0f) / 12.0f);
    }

    template <typename Duration>
    void Wait(Duration duration)
    {
        std::this_thread::sleep_for(duration);
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

        if (cmds.size() > 1) path_prefix = cmds[1] + "/";  // cmds[0] is the path to the exe

        const std::string path = path_prefix + name;
        std::shared_ptr<AudioBus> bus = MakeBusFromFile(path, false);
        if (!bus) throw std::runtime_error("couldn't open " + path);

        return bus;
    }
};

#endif
