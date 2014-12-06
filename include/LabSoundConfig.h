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

#if defined(TARGET_OS_IPHONE) || defined(TARGET_OS_MAC)
# ifndef HAVE_LIBDL
#  define HAVE_LIBDL 1
# endif
# ifndef HAVE_ALLOCA
#  define HAVE_ALLOCA 1
# endif
# ifndef HAVE_UNISTED_H
#  define HAVE_UNISTD_H 1
# endif
# ifndef USEAPI_DUMMY
#  define USEAPI_DUMMY 1
# endif
# ifndef ENABLE_MEDIA_STREAM
#  define ENABLE_MEDIA_STREAM 1
# endif
#endif

#ifndef ENABLE_WEB_AUDIO
# define ENABLE_WEB_AUDIO 1
#endif

#endif
