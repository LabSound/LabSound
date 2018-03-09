// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef FinishableSourceNode_h
#define FinishableSourceNode_h 

#include "LabSound/core/SampledAudioNode.h"

#include <functional>

namespace lab {

class FinishableSourceNode : public SampledAudioNode {
public:
  FinishableSourceNode(std::function<void(ContextRenderLock &r)> &&finishedCallback);
  ~FinishableSourceNode();
protected:
  virtual void finish(ContextRenderLock &r);

  std::function<void(ContextRenderLock &r)> finishedCallback;
};

} // namespace lab

#endif // SampledAudioNode
