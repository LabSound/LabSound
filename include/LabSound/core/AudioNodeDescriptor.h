
// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#ifndef AudioNodeDescriptor_h
#define AudioNodeDescriptor_h

namespace lab {

struct AudioParamDescriptor;
struct AudioSettingDescriptor;
struct AudioNodeDescriptor
{
    AudioParamDescriptor * params = nullptr;
    AudioSettingDescriptor * settings = nullptr;
    int initialChannelCount = 0;

    AudioParamDescriptor const * const param(char const * const) const;
    AudioSettingDescriptor const * const setting(char const * const) const;
};

} // namespace

#endif

