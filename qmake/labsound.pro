CONFIG(debug, debug|release): CONFIG -= release
else: CONFIG -= debug

QT -= core gui
CONFIG += c++11 staticlib

TEMPLATE = lib
TARGET = labsound

DEFINES -= UNICODE
DEFINES += OPUS_BUILD USE_ALLOCA

LIBS += -llibnyquist


# Platform-dependent

win32 {
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/../build/bin-win64
		LIBS += -L$$PWD/../build/bin-win64
	} else { # x32
		DESTDIR = $$PWD/../build/bin-win32
		LIBS += -L$$PWD/../build/bin-win32
	}
	
	DEFINES += __WINDOWS_WASAPI__ MODPLUG_STATIC NOMINMAX
	DEFINES += STATICALLY_LINKED_WITH_WTF
	DEFINES += _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
	DEFINES += D_VARIADIC_MAX=10 WTF_USE_WEBAUDIO_KISSFFT=1
	DEFINES += HAVE_NO_OFLOG HAVE_BOOST_THREAD HAVE_LIBDL HAVE_ALLOCA HAVE_UNISTD_H
	DEFINES += __OS_WINDOWS__ __LITTLE_ENDIAN__
	
	SOURCES += $$files($$PWD/../src/internal/src/win/*.cpp)
	SOURCES += $$PWD/../third_party/rtaudio/src/RtAudio.cpp
	SOURCES += $$PWD/../third_party/STK/src/STKInlineCompile.cpp
	
	QMAKE_LFLAGS += /NODEFAULTLIB:MSVCRT /OPT:REF
	QMAKE_CFLAGS_RELEASE = -O2 -MT
	QMAKE_CXXFLAGS_RELEASE = -O2 -MT
	QMAKE_CXXFLAGS_WARN_ON -= -w34100
	
} else:macx {
	
	DESTDIR = $$PWD/../build/bin-mac64
	LIBS += -L$$PWD/../build/bin-mac64
	
	SOURCES += $$files($$PWD/../src/internal/src/mac/*.cpp)
	
} else {
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/../build/bin-linux64
		LIBS += -L$$PWD/../build/bin-linux64
	} else { # x32
		DESTDIR = $$PWD/../build/bin-linux32
		LIBS += -L$$PWD/../build/bin-linux32
	}
	
	SOURCES += $$PWD/../third_party/rtaudio/src/RtAudio.cpp
	
}


# LabSound

INCLUDEPATH += \
	$$PWD/../include \
	$$PWD/../src \
	$$PWD/../src/internal \
	$$PWD/../third_party \
	$$PWD/../third_party/STK \
	$$PWD/../third_party/libnyquist/include


SOURCES += \
	$$files($$PWD/../src/core/*.cpp) \
	$$files($$PWD/../src/extended/*.cpp) \
	$$files($$PWD/../src/internal/src/*.cpp) \
	$$files($$PWD/../third_party/kissfft/src/*.cpp) \
	$$files($$PWD/../third_party/ooura/src/*.cpp)
