
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
                "WTF", "WTF/icu", "WTF/wtf",
                "../../ofxPd/src" }
                  
    files { 
            -- LabSound, extra processing, and binding classes
            "LabSound/*",
            "Samples/*",
            "shim/*",
            -- the webaudio engine itself
            "Modules/webaudio/*",
            "platform/audio/*",
            -- utility functions to support webaudio
            "platform/AutodrainedPool.h",
            "platform/graphics/FloatPoint3D.*",
            "platform/Logging.*",
            "platform/PlatformMemory*.*",
            -- only a small part of WTF is required to build webaudio
            "WTF/wtf/ArrayBuffer*.*",
            "WTF/wtf/Assertions.*",
            "WTF/wtf/CurrentTime.*",
            "WTF/wtf/DateMath.*", 
            "WTF/wtf/DataLog.*",
            "WTF/wtf/dtoa.*", "WTF/wtf/dtoa/*.*",
            "WTF/wtf/FastMalloc.*",
            "WTF/wtf/FilePrintStream.*",
            "WTF/wtf/Float32*.*",
            "WTF/wtf/MainThread.*",
            "WTF/wtf/MemoryInstrumentation*.*",
            "WTF/wtf/PrintStream.*",
            "WTF/wtf/StackBounds.*",
            "WTF/wtf/TCSystemAlloc.*",
            "WTF/wtf/text/CString.*", "WTF/wtf/text/AtomicString*.*", "WTF/wtf/text/StringBuilder.cpp", 
            "WTF/wtf/text/StringStatics.cpp", "WTF/wtf/text/StringImpl.*", "WTF/wtf/text/WTFString.*",
            "WTF/wtf/Threading.*", "WTF/wtf/WTFThreadData.*", "WTF/wtf/ThreadId*.*", "WTF/wtf/ThreadingP*",
            "WTF/wtf/unicode/*", "WTF/wtf/unicode/icu/*", 
            -- platform specific stuff follows
            ---- OSX
            "platform/audio/mac/*", "platform/mac/AutoDrainedPool.mm", "WTF/wtf/mac/MainThreadMac.mm" }

    excludes {
            -- these files are specific to being embedded in a browser
            "Modules/webaudio/AudioProcessingEvent.*",
            "Modules/webaudio/ScriptProcessorNode.*",
            "Modules/webaudio/OfflineAudioCompletionEvent.*" }

    libdirs { }
    
    configuration "Debug"
        targetdir "build/Debug"
        defines {  "STATICALLY_LINKED_WITH_WTF", "DEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
        flags { "Symbols" }

    configuration "Release"
        targetdir "build/Release"
        defines { "STATICALLY_LINKED_WITH_WTF", "NDEBUG", "PD", "__MACOSX_CORE__", "HAVE_NO_OFLOG", "HAVE_BOOST_THREAD", "HAVE_LIBDL", "OSX", "HAVE_ALLOCA", "HAVE_UNISTD_H", "USEAPI_DUMMY" }
        flags { "Optimize" } 

    configuration "macosx"
        buildoptions { "-std=c++11", "-stdlib=libc++" }
