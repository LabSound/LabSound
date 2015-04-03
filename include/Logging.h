//
//  Logging.h
//  LabSound
//
//  Created by Dimitri Diakopoulos on 3/16/15.
//
//

#ifndef LabSound_Logging_h
#define LabSound_Logging_h

#include <cstdarg>
#include <stdio.h>

#define LOG(...) LabSoundLog(__FILE__, __LINE__, __func__, __VA_ARGS__);

inline void LabSoundLog(const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    vprintf(fmt, args);

    fflush(stdout);
    
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