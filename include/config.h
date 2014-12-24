


#include "WTF/Platform.h"
#include "WTF/ExportMacros.h"
#include "PlatformExportMacros.h"
#include "LabSoundConfig.h"

#if OS(WINDOWS)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#ifndef WINVER
#define WINVER 0x0500
#endif

// If we don't define these, they get defined in windef.h.
// We want to use std::min and std::max.
#ifndef max
#define max max
#endif
#ifndef min
#define min min
#endif

#endif /* OS(WINDOWS) */

