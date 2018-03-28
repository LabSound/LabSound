CONFIG(debug, debug|release): CONFIG -= release
else: CONFIG -= debug

QT -= core gui
CONFIG += c++11 console

TEMPLATE = app
TARGET = examples

LIBS += -llabsound -llibnyquist

DEFINES -= UNICODE

INCLUDEPATH += \
	$$PWD/../include \
	$$PWD/../src \
	$$PWD/../src/internal \
	$$PWD/../third_party \
	$$PWD/../third_party/libnyquist/include

win32 {
	
	LIBS += -lwinmm -lole32 -luser32 -lgdi32
	
	QMAKE_CFLAGS_RELEASE = -O2 -MT
	QMAKE_CXXFLAGS_RELEASE = -O2 -MT
	QMAKE_CXXFLAGS_WARN_ON -= -w34100
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/../build/bin-win64
		LIBS += -L$$PWD/../build/bin-win64
	} else { # x32
		DESTDIR = $$PWD/../build/bin-win32
		LIBS += -L$$PWD/../build/bin-win32
	}
	
} else:macx {
	
	DESTDIR = $$PWD/../build/bin-mac64
	LIBS += -L$$PWD/../build/bin-mac64
	
} else {
	
	contains(QMAKE_TARGET.arch, x86_64) { # x64
		DESTDIR = $$PWD/../build/bin-linux64
		LIBS += -L$$PWD/../build/bin-linux64
	} else { # x32
		DESTDIR = $$PWD/../build/bin-linux32
		LIBS += -L$$PWD/../build/bin-linux32
	}
	
}


SOURCES += \
	$$PWD/../examples/src/ExamplesMain.cpp
