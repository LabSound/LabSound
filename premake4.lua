
-- awesome docs:
-- https://github.com/0ad/0ad/blob/master/build/premake/premake4.lua

-- A solution contains projects, and defines the available configurations
solution "LabSound"
   configurations { "Debug", "Release" }

   -- location is commented out because info.plist needs to be relative to xcodeproj
   -- but premake doesn't know where to find it if it is called ../osx/Info.plist
   -- location "build"
 
   -- A project defines one build target
   project "LabSound"
      kind "WindowedApp"  -- ConsoleApp, SharedLib are alternates
      language "C++"

      -- location is commented out because info.plist needs to be relative to xcodeproj
      -- but premake doesn't know where to find it if it is called ../osx/Info.plist
      -- location "build"

      platforms { "x32", "x64" }
      
      includedirs { "src/platform", 
                    "src/platform/animation", 
                    "src/platform/audio", 
                    "src/platform/graphics",
                    "src/platform/graphics/transforms",
                    "src/platform/text",
                    "src/Modules/webaudio",
                    "src/shim",
                    "src/WTF", "src/WTF/icu" }
      
      files { "**.h",     -- ** means recurse
              "**.cpp",
              "**.c",
              "**.mm",
              "osx/Info.plist",
              "src/platform/audio/resources/**",
              "Resources/**" }
              
      excludes { "src/WTF/build/**", 
                "src/WTF/Configurations/**", 
                "src/WTF/WTF.gyp/**", 
                "src/WTF/WTF.vcproj/**", 
                "src/WTF/wtf/blackberry/**",
                "src/WTF/wtf/chromium/**", 
                "src/WTF/wtf/efl/**", 
                "src/WTF/wtf/gobject/**", 
                "src/WTF/wtf/gtk/**", 
                "src/WTF/wtf/qt/**", 
                "src/WTF/wtf/win/**", 
                "src/WTF/wtf/wince/**", 
                "src/WTF/wtf/wx/**", 
                "src/audio/qt/**", 
                "src/audio/win/**", 
                "src/audio/wince/**", 
                "src/audio/wx/**", 
                "src/platform/animation/Animation.cpp",
                "src/platform/animation/AnimationList.cpp",
                "src/platform/audio/chromium/**",
                "src/platform/audio/efl/**",
                "src/platform/audio/ffmpeg/**",
                "src/platform/audio/gstreamer/**",
                "src/platform/audio/gtk/**",
                "src/platform/audio/ipp/**",
                "src/platform/audio/mkl/**",
                "src/platform/audio/qt/**",
                "src/Modules/webaudio/AudioProcessingEvent.cpp",
                "src/Modules/webaudio/ScriptProcessorNode.cpp",
                "src/Modules/webaudio/OfflineAudioCompletionEvent.cpp",
                "src/WTF/wtf/**GLib.cpp",
                "src/WTF/wtf/MemoryManager.cpp",
                "src/WTF/wtf/**Wx.cpp",
                "src/WTF/wtf/**Win.cpp"
                }

      links { "icucore", "WTF",
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
              --  os.findlib("X11") placeholder how to find system library
              }

        libdirs { "src/WTF/build/Release" }
        
        configuration "Debug"
            targetdir "build/Debug"
            defines {  "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Symbols" }
 
        configuration "Release"
            targetdir "build/Release"
            defines { "ENABLE_MEDIA_STREAM=1", "ENABLE_WEB_AUDIO=1", "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
            flags { "Optimize" } 
