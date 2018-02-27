CONFIG(debug, debug|release): CONFIG -= release
else: CONFIG -= debug

QT -= core gui
CONFIG += c++11 staticlib

TEMPLATE = lib
TARGET = labsound

DEFINES -= UNICODE
DEFINES += OPUS_BUILD USE_ALLOCA

INCLUDEPATH += \
	$$PWD/../include \
	$$PWD/../src \
	$$PWD/../src/internal \
	$$PWD/../third_party \
	$$PWD/../third_party/libnyquist/include


# libopus
INCLUDEPATH += \
$$PWD/../third_party/libnyquist/third_party/libogg/include \
$$PWD/../third_party/libnyquist/third_party/opus/celt \
$$PWD/../third_party/libnyquist/third_party/opus/libopus/include \
$$PWD/../third_party/libnyquist/third_party/opus/opusfile/include \
$$PWD/../third_party/libnyquist/third_party/opus/opusfile/src/include \
$$PWD/../third_party/libnyquist/third_party/opus/silk \
$$PWD/../third_party/libnyquist/third_party/opus/silk/float


#libnyquist
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


win32 {
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/../build/bin-win64
	} else { # x32
		DESTDIR = $$PWD/../build/bin-win32
	}
	
	DEFINES += __WINDOWS_WASAPI__=1 MODPLUG_STATIC
	
	SOURCES += $$files($$PWD/../src/internal/src/win/*.cpp)
	SOURCES += $$PWD/../third_party/rtaudio/src/RtAudio.cpp
	
} else:macx {
	
	DESTDIR = $$PWD/../build/bin-mac64
	
	SOURCES += $$files($$PWD/../src/internal/src/mac/*.cpp)
	
} else {
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/../build/bin-linux64
	} else { # x32
		DESTDIR = $$PWD/../build/bin-linux32
	}
	
	SOURCES += $$PWD/../third_party/rtaudio/src/RtAudio.cpp
	
}


SOURCES += \
	$$files($$PWD/../src/core/*.cpp) \
	$$files($$PWD/../src/extended/*.cpp) \
	$$files($$PWD/../src/internal/src/*.cpp) \
	$$files($$PWD/../third_party/kissfft/src/*.cpp)


# libopus
SOURCES += \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/celt/*.c) \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/libopus/src/*.c) \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/opusfile/src/*.c) \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/silk/*.c) \
	$$files($$PWD/../third_party/libnyquist/third_party/opus/silk/float/*.c)


# libnyquist
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

