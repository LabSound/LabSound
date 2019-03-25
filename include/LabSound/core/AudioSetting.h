
// License: BSD 3 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef AudioSetting_h
#define AudioSetting_h

#include <functional>
#include <string>
#include <cstdint>

namespace lab
{

// value defaults to zero, assumed set as integer.
// floatAssigned() is provided so that a user interface or RPC binding can configure
// automatically for float or unsigned integer values.
//
// a value changed callback is provided so that if tools set values, they can
// be responded to.
//
class AudioSetting
{
    std::string _name;
    float _valf = 0;
    uint32_t _vali = 0;
    bool _asFloat = false;
    std::function<void()> _valueChanged;

public:
    explicit AudioSetting(const std::string & n)
    : _name(n)
    {
    }
    explicit AudioSetting(char const * const n)
    : _name(n)
    {
    }
    
    void setValueChanged(std::function<void()> fn) { _valueChanged = fn; }
    std::string name() const { return _name; }

    float valueFloat() const { return _valf; }
    void setFloat(float v, bool notify = true)
    {
        if (v == _valf)
            return;

        _valf = v;
        _vali = uint32_t(v);
        _asFloat = true;
        if (notify && _valueChanged) _valueChanged();
    }

    uint32_t valueUint32() const { return _vali; }
    void setUint32(uint32_t v, bool notify = true)
    {
        if (v == _vali)
            return;

        _vali = v;
        _valf = float(v);
        _asFloat = false;
        if (notify && _valueChanged) _valueChanged();
    }

    bool floatAssigned() const { return _asFloat; }
};

} // lab

#endif
