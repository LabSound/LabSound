// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.
//
// audiotexture - a command line utility that renders an audio file to a PNG.
//
// The input signal is split into three frequency bands (low / band / high),
// each band is compressed, and the three levels are mapped to R/G/B of a
// time-vs-magnitude image (see AudioTextureFixture / TextureRecorderNode).
//
// Usage:
//   audiotexture <input-audio> [output.png] [width] [height] [mirror]
//
//   input-audio  path to a wav/mp3/ogg/flac/... file (decoded by libnyquist)
//   output.png   output image path            (default: <input>.png)
//   width        image width  in pixels        (default: 1024)
//   height       image height in pixels        (default: 256)
//   mirror       0 or 1, symmetric "pulse" fill (default: 0)

#include "LabSound/LabSound.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

using namespace lab;

static void print_usage(const char * argv0)
{
    printf("usage: %s <input-audio> [output.png] [width] [height] [mirror] [overlap] [gamma]\n", argv0);
    printf("  input-audio  path to an audio file (wav/mp3/ogg/flac/...)\n");
    printf("  output.png   output image path            (default: <input>.png)\n");
    printf("  width        image width  in pixels       (default: 1024)\n");
    printf("  height       image height in pixels       (default: 256)\n");
    printf("  mirror       0 or 1, symmetric pulse fill (default: 0)\n");
    printf("  overlap      RMS window smoothing, >=1.0  (default: 2.0)\n");
    printf("  gamma        color shadow-lift, >1 lifts  (default: 1.5, 1.0=off)\n");
}

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    const std::string inputPath = argv[1];

    std::string outputPath;
    if (argc >= 3)
    {
        outputPath = argv[2];
    }
    else
    {
        // default: strip a trailing extension from the input and append .png
        const size_t dot = inputPath.find_last_of('.');
        const size_t slash = inputPath.find_last_of("/\\");
        if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
            outputPath = inputPath.substr(0, dot) + ".png";
        else
            outputPath = inputPath + ".png";
    }

    const int width = (argc >= 4) ? std::atoi(argv[3]) : 1024;
    const int height = (argc >= 5) ? std::atoi(argv[4]) : 256;
    const bool mirror = (argc >= 6) ? (std::atoi(argv[5]) != 0) : false;
    const float overlap = (argc >= 7) ? static_cast<float>(std::atof(argv[6])) : 2.0f;
    const float gamma = (argc >= 8) ? static_cast<float>(std::atof(argv[7])) : 1.5f;

    if (width <= 0 || height <= 0)
    {
        printf("error: width and height must be positive\n");
        return 1;
    }

    // Offline context: the graph is pulled by a null destination device rather
    // than a real audio device, so no sound hardware is touched.
    auto context = std::make_shared<lab::AudioContext>(true /*isOffline*/, false /*autoDispatchEvents*/);

    // A mono null destination keeps the graph channel counts consistent with
    // the fixture's mono texture-recorder sink.
    AudioStreamConfig offlineConfig;
    offlineConfig.device_index = 0;
    offlineConfig.desired_samplerate = LABSOUND_DEFAULT_SAMPLERATE;
    offlineConfig.desired_channels = 1;
    AudioStreamConfig inputConfig = {};

    auto renderNode = std::make_shared<lab::AudioDestinationNode>(
        *context, std::make_unique<lab::AudioDevice_Null>(inputConfig, offlineConfig));
    context->setDestinationNode(renderNode);

    // Load and decode the input file, mixed to mono.
    std::shared_ptr<AudioBus> clip = MakeBusFromFile(inputPath, true /*mixToMono*/);
    if (!clip)
    {
        printf("error: could not load audio file '%s'\n", inputPath.c_str());
        return 1;
    }

    // Build source -> 3 filters -> 3 compressors -> texture recorder -> destination.
    auto fixture = std::make_shared<lab::AudioTextureFixture>(*context, width, height, mirror);
    fixture->textureRecorder()->setOverlap(overlap);
    fixture->textureRecorder()->setColorGamma(gamma);

    std::shared_ptr<SampledAudioNode> clipNode;
    {
        ContextRenderLock r(context.get(), "audiotexture");
        clipNode = std::make_shared<SampledAudioNode>(*context);
        clipNode->setBus(clip);
        fixture->connectInput(*context, clipNode);
        clipNode->schedule(0.0);
    }

    fixture->startRecording();
    auto scratch = std::make_shared<lab::AudioBus>(1, AudioNode::ProcessingSizeInFrames);
    renderNode->offlineRender(scratch.get(), clip->length());
    fixture->stopRecording();

    printf("rendered %.3f seconds of audio (%d x %d%s)\n",
           fixture->textureRecorder()->recordedLengthInSeconds(),
           width, height, mirror ? ", mirrored" : "");

    if (fixture->writePNG(outputPath))
    {
        printf("wrote %s\n", outputPath.c_str());
        return 0;
    }

    printf("error: failed to write %s\n", outputPath.c_str());
    return 1;
}
