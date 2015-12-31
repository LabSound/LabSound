// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef LABSOUND_UTIL_H
#define LABSOUND_UTIL_H

#define NO_COPY(C) C(const C &) = delete; C & operator = (const C &) = delete
#define NO_MOVE(C) NO_COPY(C); C(C &&) = delete; C & operator = (const C &&) = delete

#endif
