CONFIG(debug, debug|release): CONFIG -= release
else: CONFIG -= debug

QT -= core gui
CONFIG += c++11 staticlib

TEMPLATE = lib
TARGET = libnyquist

DEFINES -= UNICODE
DEFINES += OPUS_BUILD USE_ALLOCA


# Platform-dependent

win32 {
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/../build/bin-win64
	} else { # x32
		DESTDIR = $$PWD/../build/bin-win32
	}
	
	DEFINES += __WINDOWS_WASAPI__ MODPLUG_STATIC NOMINMAX
	DEFINES += STATICALLY_LINKED_WITH_WTF
	DEFINES += _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
	DEFINES += D_VARIADIC_MAX=10 WTF_USE_WEBAUDIO_KISSFFT=1
	DEFINES += HAVE_NO_OFLOG HAVE_BOOST_THREAD HAVE_LIBDL HAVE_ALLOCA HAVE_UNISTD_H
	DEFINES += __OS_WINDOWS__ __LITTLE_ENDIAN__
	
	QMAKE_LFLAGS += /NODEFAULTLIB:MSVCRT /OPT:REF
	QMAKE_CFLAGS_RELEASE = -O2 -MT
	QMAKE_CXXFLAGS_RELEASE = -O2 -MT
	QMAKE_CXXFLAGS_WARN_ON -= -w34100
	
} else:macx {
	
	DESTDIR = $$PWD/../build/bin-mac64
	
} else {
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/../build/bin-linux64
	} else { # x32
		DESTDIR = $$PWD/../build/bin-linux32
	}
	
}


# libopus

INCLUDEPATH += \
	$$PWD/../third_party/libnyquist/third_party/libogg/include \
	$$PWD/../third_party/libnyquist/third_party/opus/celt \
	$$PWD/../third_party/libnyquist/third_party/opus/libopus/include \
	$$PWD/../third_party/libnyquist/third_party/opus/opusfile/include \
	$$PWD/../third_party/libnyquist/third_party/opus/opusfile/src/include \
	$$PWD/../third_party/libnyquist/third_party/opus/silk \
	$$PWD/../third_party/libnyquist/third_party/opus/silk/float

SOURCES += \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/celt/*.c) \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/libopus/src/*.c) \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/opusfile/src/*.c) \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/silk/*.c) \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/silk/float/*.c)

SOURCES -= \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/libopus/src/*_demo.c) \
	$$PWD/../third_party/libnyquist/third_party/opus/libopus/src/opus_compare.c \
	$$PWD/../third_party/libnyquist/third_party/opus/celt/opus_custom_demo.c


# libnyquist

INCLUDEPATH += \
	$$PWD/../third_party/libnyquist/include \
	$$PWD/../third_party/libnyquist/include/libnyquist \
	$$PWD/../third_party/libnyquist/third_party \
	$$PWD/../third_party/libnyquist/third_party/FLAC/src/include \
	$$PWD/../third_party/libnyquist/third_party/libogg/include \
	$$PWD/../third_party/libnyquist/third_party/libvorbis/include \
	$$PWD/../third_party/libnyquist/third_party/libvorbis/src \
	$$PWD/../third_party/libnyquist/third_party/musepack/include \
	$$PWD/../third_party/libnyquist/third_party/opus/celt \
	$$PWD/../third_party/libnyquist/third_party/opus/libopus/include \
	$$PWD/../third_party/libnyquist/third_party/opus/opusfile/include \
	$$PWD/../third_party/libnyquist/third_party/opus/opusfile/src/include \
	$$PWD/../third_party/libnyquist/third_party/opus/silk \
	$$PWD/../third_party/libnyquist/third_party/opus/silk/float \
	$$PWD/../third_party/libnyquist/third_party/wavpack/include \
	$$PWD/../third_party/libnyquist/src

SOURCES += \
	$$PWD/../third_party/libnyquist/src/AudioDecoder.cpp \
	$$PWD/../third_party/libnyquist/src/CafDecoder.cpp \
	$$PWD/../third_party/libnyquist/src/Common.cpp \
	$$PWD/../third_party/libnyquist/src/FlacDecoder.cpp \
	$$PWD/../third_party/libnyquist/src/FlacDependencies.c \
	$$PWD/../third_party/libnyquist/src/ModplugDecoder.cpp \
	$$PWD/../third_party/libnyquist/src/ModplugDependencies.cpp \
	$$PWD/../third_party/libnyquist/src/MusepackDecoder.cpp \
	$$PWD/../third_party/libnyquist/src/MusepackDependencies.c \
	$$PWD/../third_party/libnyquist/src/OpusDecoder.cpp \
	$$PWD/../third_party/libnyquist/src/OpusDependencies.c \
	$$PWD/../third_party/libnyquist/src/RiffUtils.cpp \
	$$PWD/../third_party/libnyquist/src/VorbisDecoder.cpp \
	$$PWD/../third_party/libnyquist/src/VorbisDependencies.c \
	$$PWD/../third_party/libnyquist/src/WavDecoder.cpp \
	$$PWD/../third_party/libnyquist/src/WavEncoder.cpp \
	$$PWD/../third_party/libnyquist/src/WavPackDecoder.cpp \
	$$PWD/../third_party/libnyquist/src/WavPackDependencies.c
