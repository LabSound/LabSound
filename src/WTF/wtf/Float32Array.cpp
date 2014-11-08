//
//  Float32Array.cpp
//  WTF
//
//  Created by Nick Porcino on 2012 12/1.
//
//

#include "WTF\Float32Array.h"

namespace WTF {

    ArrayBufferView::ViewType Float32Array::getType() const
    {
        return TypeFloat32;
    }

}
