/*
 *
 * Technically these macros were originally part of the WTF library:
 *
 * Copyright (C) 2003, 2006, 2007, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LabSound_Assertions_h
#define LabSound_Assertions_h

#include "Platform.h"
#include "ExportMacros.h"
#include "Logging.h"

// FIXME: Change to use something other than ASSERT to avoid this conflict with the underlying platform
#if OS(WINDOWS)
#undef ASSERT
#endif

#define CRASH() ((void)0)

#define ASSERT(assertion)                                       \
(!(assertion) ?                                                 \
(LabSoundAssertLog(__FILE__, __LINE__, __func__, #assertion),   \
CRASH()) :                                                      \
(void)0)

#define ASSERT_AT(assertion, file, line, function)              \
(!(assertion) ?                                                 \
(LabSoundAssertLog(file, line, function, #assertion),           \
CRASH()) :                                                      \
(void)0)

#define ASSERT_NOT_REACHED() do {                               \
LabSoundAssertLog(__FILE__, __LINE__, __func__, 0);             \
CRASH() ;                                                       \
} while (0)

#define ASSERT_WITH_MESSAGE(assertion, ...) do                  \
if (!(assertion)) {                                             \
LabSoundAssertLog(__FILE__, __LINE__, __func__, #assertion); 	\
CRASH() ;                                                       \
} while (0)

#define FATAL(...) do {                                         \
LabSoundLog(__FILE__, __LINE__, __VA_ARGS__);                   \
CRASH() ;                                                       \
} while (0)

#define ASSERT_UNUSED(variable, assertion) ASSERT(assertion)

#define ASSERT_WITH_SECURITY_IMPLICATION(assertion) ASSERT(assertion)

#define LOG_ERROR(...) LabSoundLog(__FILE__, __LINE__, __VA_ARGS__)

#define LOG_VERBOSE(channel, ...) LabSoundLog(__FILE__, __LINE__, __VA_ARGS__)

#endif
