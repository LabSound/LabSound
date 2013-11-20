//
//  LabSoundConfig.h
//  LabSound
//
//  Created by Nick Porcino on 2013 11/19.
//
//

#ifndef LabSound_LabSoundConfig_h
#define LabSound_LabSoundConfig_h

// Configure Webaudio features
#ifndef ENABLE_MEDIA_STREAM
# define ENABLE_MEDIA_STREAM 1
#endif
#ifndef ENABLE_MEDIA_STREAM
# define ENABLE_WEB_AUDIO 1
#endif
#ifndef STATICALLY_LINKED_WITH_WTF
# define STATICALLY_LINKED_WITH_WTF 1
#endif

// Pd features
#ifndef PD
# define PD 1
#endif
#if !defined(HAVE_LIBDL) && (defined(TARGET_OS_IPHONE) || defined(TARGET_OS_MAC))
# define HAVE_LIBDL 1
# define HAVE_ALLOCA 1
# define HAVE_UNISTD_H 1
# define USEAPI_DUMMY 1
#endif

#endif
