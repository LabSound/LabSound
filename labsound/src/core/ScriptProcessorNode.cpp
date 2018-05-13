// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

// @tofix - webkit change ef113b changes the logic of processing to test for non finite time values, change not reflected here.
// @tofix - webkit change e369924 adds backward playback, change not incorporated yet

#include "LabSound/core/ScriptProcessorNode.h"

using namespace std;

namespace lab {

ScriptProcessorNode::ScriptProcessorNode(unsigned int bufferSize, unsigned int numChannels, std::function<void(lab::ContextRenderLock& r, std::vector<const float*> sources, std::vector<float*> destinations, size_t framesToProcess)> &&kernel) : kernel(std::move(kernel)) {
  addInput(std::unique_ptr<AudioNodeInput>(new AudioNodeInput(this, bufferSize)));
  addOutput(std::unique_ptr<AudioNodeOutput>(new AudioNodeOutput(this, numChannels, bufferSize)));

  initialize();
}
ScriptProcessorNode::~ScriptProcessorNode() {}
void ScriptProcessorNode::pullInputs(ContextRenderLock& r, size_t framesToProcess) {
  // Render input stream - suggest to the input to render directly into output bus for in-place processing in process() if possible.
  input(0)->pull(r, output(0)->bus(r), framesToProcess);
}
void ScriptProcessorNode::reset(ContextRenderLock &r) {}
// As soon as we know the channel count of our input, we can lazily initialize.
// Sometimes this may be called more than once with different channel counts, in which case we must safely
// uninitialize and then re-initialize with the new channel count.
void ScriptProcessorNode::checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input)
{
    if (input != this->input(0).get())
        return;

    unsigned numberOfChannels = input->numberOfChannels(r);

    bool mustPropagate = false;
    for (size_t i = 0; i < numberOfOutputs() && !mustPropagate; ++i) {
        mustPropagate = isInitialized() && numberOfChannels != output(i)->numberOfChannels();
    }

    if (mustPropagate) {
        uninitialize();
        for (size_t i = 0; i < numberOfOutputs(); ++i) {
            // This will propagate the channel count to any nodes connected further down the chain...
            output(i)->setNumberOfChannels(r, numberOfChannels);
        }
        initialize();
    }

    AudioNode::checkNumberOfChannelsForInput(r, input);
}
double ScriptProcessorNode::tailTime(ContextRenderLock & r) const {
  return 0;
}
double ScriptProcessorNode::latencyTime(ContextRenderLock & r) const {
  return 0;
}
void ScriptProcessorNode::process(ContextRenderLock& r, size_t framesToProcess)
{
    AudioBus* destinationBus = output(0)->bus(r);

    if (!isInitialized())
        destinationBus->zero();
    else {
        AudioBus* sourceBus = input(0)->bus(r);

        // FIXME: if we take "tail time" into account, then we can avoid calling processor()->process() once the tail dies down.
        if (!input(0)->isConnected())
            sourceBus->zero();

        process(r, sourceBus, destinationBus, framesToProcess);
    }
}
void ScriptProcessorNode::process(ContextRenderLock& r, const AudioBus* source, AudioBus* destination, size_t framesToProcess) {
  // ASSERT(source && destination);
  if (!source || !destination) {
    return;
  }

  if (!isInitialized()) {
    destination->zero();
    return;
  }

  bool channelCountMatches = source->numberOfChannels() == destination->numberOfChannels();
  // ASSERT(channelCountMatches);
  if (!channelCountMatches) {
    return;
  }

  size_t numChannels = source->numberOfChannels();
  std::vector<const float*> sources(numChannels);
  std::vector<float*> destinations(numChannels);
  for (unsigned i = 0; i < numChannels; ++i) {
    sources[i] = source->channel(i)->data();
    destinations[i] = destination->channel(i)->mutableData();
  }

  kernel(r, sources, destinations, framesToProcess);
}

} // namespace lab

