// License: BSD 3 Clause
// Copyright (C) 2007, Apple Inc. All rights reserved.
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef FloatConversion_h
#define FloatConversion_h

namespace lab
{

    template<typename T>
    float narrowPrecisionToFloat(T);

    template<>
    inline float narrowPrecisionToFloat(double number)
    {
        return static_cast<float>(number);
    }

} // namespace lab

#endif // FloatConversion_h
