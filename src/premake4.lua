
project "LabSound"
    kind "StaticLib"  -- ConsoleApp, SharedLib are alternates
    language "C++"
    
    platforms { "x32", "x64" }
    
    includedirs { "platform", 
                "platform/animation", 
                "platform/audio", 
                "platform/graphics",
                "platform/graphics/transforms",
                "platform/text",
                "Modules/webaudio",
                "shim",
                "WTF", "WTF/icu" }
    
    files { "**.h",     -- ** means recurse
          "**.cpp",
          "**.cc",
          "**.c",
          "**.mm" }
          
    excludes { "WTF/build/**", 
            "WTF/Configurations/**", 
            "WTF/WTF.gyp/**", 
            "WTF/WTF.vcproj/**", 
            "WTF/wtf/blackberry/**",
            "WTF/wtf/chromium/**", 
            "WTF/wtf/efl/**", 
            "WTF/wtf/gobject/**", 
            "WTF/wtf/gtk/**", 
            "WTF/wtf/qt/**", 
            "WTF/wtf/win/**", 
            "WTF/wtf/wince/**", 
            "WTF/wtf/wx/**", 
            "audio/qt/**", 
            "audio/win/**", 
            "audio/wince/**", 
            "audio/wx/**", 
            "platform/animation/Animation.cpp",
            "platform/animation/AnimationList.cpp",
            "platform/audio/chromium/**",
            "platform/audio/efl/**",
            "platform/audio/ffmpeg/**",
            "platform/audio/gstreamer/**",
            "platform/audio/gtk/**",
            "platform/audio/ipp/**",
            "platform/audio/mkl/**",
            "platform/audio/qt/**",
            "Modules/webaudio/AudioProcessingEvent.cpp",
            "Modules/webaudio/ScriptProcessorNode.cpp",
            "Modules/webaudio/OfflineAudioCompletionEvent.cpp",
            "WTF/wtf/**GLib.cpp",
            "WTF/wtf/MemoryManager.cpp",
            "WTF/wtf/**Wx.cpp",
            "WTF/wtf/**Win.cpp",
            "SampleApps/**.*"
            }

    libdirs { }
    
    configuration "Debug"
        targetdir "build/Debug"
        defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
        flags { "Symbols" }

    configuration "Release"
        targetdir "build/Release"
        defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
        flags { "Optimize" } 

    configuration "macosx"
        buildoptions { "-std=c++11", "-stdlib=libc++" }
