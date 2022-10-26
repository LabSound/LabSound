//
// License: BSD 3 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.
//

#ifndef AUDIOPARAMDESCRIPTOR_H
#define AUDIOPARAMDESCRIPTOR_H

namespace lab {

struct AudioParamDescriptor
{
    char const * const name;
    char const * const shortName;
    double defaultValue, minValue, maxValue;
};

} // namespace

#endif
