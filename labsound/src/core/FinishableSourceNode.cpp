// License: BSD 2 Clause
// Copyright (C) 2010, Google Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

// @tofix - webkit change ef113b changes the logic of processing to test for non finite time values, change not reflected here.
// @tofix - webkit change e369924 adds backward playback, change not incorporated yet

#include "LabSound/core/FinishableSourceNode.h"

using namespace std;

namespace lab {

FinishableSourceNode::FinishableSourceNode(std::function<void()> &&finishedCallback) : finishedCallback(std::move(finishedCallback)) {}
FinishableSourceNode::~FinishableSourceNode() {}
void FinishableSourceNode::finish(ContextRenderLock &r) {
  SampledAudioNode::finish(r);

  finishedCallback();
}
} // namespace lab

