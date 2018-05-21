// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef ScriptProcessorNode_h
#define ScriptProcessorNode_h 

#include "LabSound/core/AudioNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

#include <functional>

namespace lab {

class ScriptProcessorNode : public AudioNode {
public:
  ScriptProcessorNode(unsigned int numChannels, std::function<void(lab::ContextRenderLock& r, std::vector<const float*> sources, std::vector<float*> destinations, size_t framesToProcess)> &&kernel);
  virtual ~ScriptProcessorNode();

  virtual void process(ContextRenderLock&, size_t framesToProcess) override;
  void process(ContextRenderLock& r, const AudioBus* source, AudioBus* destination, size_t framesToProcess);
  virtual void pullInputs(ContextRenderLock&, size_t framesToProcess) override;
  virtual void reset(ContextRenderLock&) override;
  virtual void checkNumberOfChannelsForInput(ContextRenderLock& r, AudioNodeInput* input) override;

protected:
  virtual double tailTime(ContextRenderLock & r) const override;
  virtual double latencyTime(ContextRenderLock & r) const override;
  virtual bool propagatesSilence(ContextRenderLock & r) const override;

  std::function<void(lab::ContextRenderLock& r, std::vector<const float*> sources, std::vector<float*> destinations, size_t framesToProcess)> kernel;
};

} // namespace lab

#endif // SampledAudioNode
