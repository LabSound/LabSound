
// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#ifndef AudioSettingDescriptor_h
#define AudioSettingDescriptor_h

namespace lab {

enum class SettingType
{
    None,
    Bool,
    Integer,
    Float,
    Enum,
    Bus
};

struct AudioSettingDescriptor
{
    char const * const name;
    char const * const shortName;
    SettingType type = SettingType::None;
    char const * const * enums = nullptr;
};

} // namespace

#endif

