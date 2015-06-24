#ifndef LabSound_Logging_h
#define LabSound_Logging_h

#include <cstdarg>
#include <stdio.h>
#include <string>
#include <iostream>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define LOG(...) LabSoundLog(__FILE__, __LINE__, __VA_ARGS__);

inline void LabSoundLog(const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    char tmp[256] = { 0 };
    sprintf(tmp, "[%s @ %i]\n\t%s\n", file, line, fmt);
    vprintf(tmp, args);
    
    va_end(args);
}

inline void LabSoundAssertLog(const char* file, int line, const char * function, const char * assertion)
{
    if (assertion)
        printf("[%s @ %i] ASSERTION FAILED: %s\n", file, line, assertion);
    else
        printf("[%s @ %i] SHOULD NEVER BE REACHED\n", file, line);
}

#endif
