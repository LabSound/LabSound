// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2012, Intel Corporation. All rights reserved.
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#include "LabSound/core/AudioBasicInspectorNode.h"
#include "LabSound/core/AnalyserNode.h"
#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioNodeInput.h"
#include "LabSound/core/AudioNodeOutput.h"

namespace lab
{

AudioBasicInspectorNode::AudioBasicInspectorNode(AudioContext & ac, AudioNodeDescriptor const & desc)
: AudioNode(ac, desc)
{
    _self->desiredChannelCount = desc.initialChannelCount;
    initialize();
}

}  // namespace lab
