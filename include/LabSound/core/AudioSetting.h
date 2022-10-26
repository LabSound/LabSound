
// SPDX-License-Identifier: BSD-2-Clause
// Copyright (C) 2015, The LabSound Authors. All rights reserved.

#ifndef AudioSetting_h
#define AudioSetting_h

#include "LabSound/core/AudioBus.h"
#include "LabSound/core/AudioSettingDescriptor.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace lab
{
// An AudioSetting holds settings for a node that don't vary with
// time, and that cannot be driven by an audio bus.
// The WebAudio interface typically exposes settings via functional
// entry points only. Exposing settings via a consistent interface
// allows consistent access through tools and bindings without
// needing to write specialized code for each setting.
//
// value defaults to Integer and zero.
//
// Applications can set the value changed callback.
//



class AudioSetting
{
public:

private:
    AudioSettingDescriptor const*const _desc;

    float _valf = 0;
    uint32_t _vali = 0;
    bool _valb = false;
    std::shared_ptr<AudioBus> _valBus;

    std::function<void()> _valueChanged;

public:
    explicit AudioSetting(AudioSettingDescriptor const * const d)
        : _desc(d)
    {
    }

    ~AudioSetting() = default;

    std::string name() const { return _desc->name; }
    std::string shortName() const { return _desc->shortName; }
    SettingType type() const { return _desc->type; }
    char const * const * enums() const { return _desc->enums; }

    int enumFromName(char const* const e)
    {
        if (!_desc->enums || !e)
            return -1;

        int enum_idx = 0;
        for (char const* const* names_p = _desc->enums; *names_p != nullptr; ++names_p, ++enum_idx)
        {
            if (!strcmp(e, *names_p))
                return enum_idx;
        }
        return -1;
    }

    bool valueBool() const { return _valb; }
    float valueFloat() const { return _valf; }
    uint32_t valueUint32() const { return _vali; }
    std::shared_ptr<AudioBus> valueBus() const { return _valBus; }

    void setBool(bool v, bool notify = true)
    {
        if (v == _valb) return;
        _valb = v;
        if (notify && _valueChanged) _valueChanged();
    }

    void setFloat(float v, bool notify = true)
    {
        if (v == _valf) return;
        _valf = v;
        if (notify && _valueChanged) _valueChanged();
    }

    void setUint32(uint32_t v, bool notify = true)
    {
        if (v == _vali) return;
        _vali = v;
        if (notify && _valueChanged) _valueChanged();
    }

    void setEnumeration(int v, bool notify = true)
    {
        if (v == _vali) return;
        if (v < 0) return;
        _vali = static_cast<int>(v);
        if (notify && _valueChanged) _valueChanged();
    }

    void setEnumeration(char const*const v, bool notify = true)
    {
        if (v) setEnumeration(enumFromName(v), notify);
    }

    void setString(char const*const, bool notify = true)
    {
        if (notify && _valueChanged) _valueChanged();
    }

    // nb: Invoking setBus will create and cache a duplicate of the supplied bus.
    void setBus(const AudioBus * incoming, bool notify = true)
    {
        std::unique_ptr<AudioBus> new_bus = AudioBus::createByCloning(incoming);
        _valBus = std::move(new_bus);
        if (notify && _valueChanged)
            _valueChanged();
    }

    void setValueChanged(std::function<void()> fn) { _valueChanged = fn; }
};

}  // namespace lab

#endif
