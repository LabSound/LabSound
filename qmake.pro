CONFIG(debug, debug|release): CONFIG -= release
else: CONFIG -= debug

QT -= core gui
CONFIG += c++11 staticlib

TEMPLATE = lib
TARGET = labsound


INCLUDEPATH += \
	$$PWD/include \
	$$PWD/src \
	$$PWD/src/internal \
	$$PWD/third_party \
	$$PWD/third_party/libnyquist/include

win32 {
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/build/bin-win64
	} else { # x32
		DESTDIR = $$PWD/build/bin-win32
	}
	
	DEFINES += __WINDOWS_WASAPI__=1
	
	SOURCES += $$PWD/src/internal/src/win/*.cpp
	
} else:macx {
	
	DESTDIR = $$PWD/build/bin-mac64
	
	SOURCES += $$PWD/src/internal/src/mac/*.cpp
	
	
} else {
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/build/bin-linux64
	} else { # x32
		DESTDIR = $$PWD/build/bin-linux32
	}
	
}

!macx {
	
	SOURCES += $$PWD/third_party/rtaudio/src/RtAudio.cpp
	
}


SOURCES += \
	$$PWD/src/core/* \
	$$PWD/src/extended/* \
	$$PWD/src/internal/src/* \
	$$PWD/third_party/kissfft/src/*
