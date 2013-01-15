
-- awesome docs:
-- https://github.com/0ad/0ad/blob/master/build/premake/premake4.lua

-- A solution contains projects, and defines the available configurations
solution "LabSound"
   configurations { "Debug", "Release" }
 
   project "DalekVoiceApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/DalekVoiceApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }
    
   project "LiveEchoApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/LiveEchoApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }
    
   project "LiveReverbRecordingApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/LiveReverbRecordingApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }

   project "ReverbApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/ReverbApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }

   project "RhythmApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/RhythmApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }

   project "RhythmFilteredApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/RhythmFilteredApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }

   project "RhythmTonePanningApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/RhythmTonePanningApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }

   project "SampleSpatializationApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/SampleSpatializationApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }

   project "ToneAndSampleApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/ToneAndSampleApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }

   project "ToneAndSampleRecordedApp"
        kind "WindowedApp"
        language "C++"
        platforms { "x32", "x64" }

        includedirs 
        {
              "src/WTF", "src/WTF/icu"
            , "src/platform/audio"
            , "src/platform/graphics"
            , "src/Modules/webaudio"
            , "src/shim"
            , "src/LabSound"
            , "src/Samples"
        }
      
        files 
        { 
            "src/SampleApps/ToneAndSampleRecordedApp.cpp"
          , "src/SampleApps/osx/Info.plist"
          , "src/platform/audio/resources/**"
        }
      
        links { 
            "icucore",
            "Accelerate.framework",
            "ApplicationServices.framework",
            "AudioToolbox.framework",
            "AudioUnit.framework",
            "Carbon.framework",
            "Cocoa.framework",
            "CoreAudio.framework",
            "CoreFoundation.framework",
            "CoreMIDI.framework",
            "CoreVideo.framework",
            "OpenGL.framework",
            "QTKit.framework",
            "LabSound"
            --  os.findlib("X11") placeholder how to find system library
        }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
        
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
            
        configuration "macosx"
            linkoptions  { "-std=c++11", "-stdlib=libc++" }
            buildoptions { "-std=c++11", "-stdlib=libc++" }


include "src"
