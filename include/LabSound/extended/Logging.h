// License: BSD 2 Clause
// Copyright (C) 2015+, The LabSound Authors. All rights reserved.

#ifndef LABSOUND_LOGGING_H
#define LABSOUND_LOGGING_H

#include <cstdarg>
#include <stdio.h>
#include <string>
#include <iostream>

#if defined(_DEBUG) || defined (DEBUG)
    #define LOG(...) LabSoundLog(__FILE__, __LINE__, __VA_ARGS__);
    #define LOG_ERROR(...) LabSoundLog(__FILE__, __LINE__, __VA_ARGS__)
    #define LOG_VERBOSE(channel, ...) LabSoundLog(__FILE__, __LINE__, __VA_ARGS__)
#else
    #define LOG(...)
    #define LOG_ERROR(channel, ...)
    #define LOG_VERBOSE(channel, ...)
#endif

void LabSoundLog(const char* file, int line, const char* fmt, ...);
void LabSoundAssertLog(const char* file, int line, const char * function, const char * assertion);

#endif
