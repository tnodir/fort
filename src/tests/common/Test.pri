QT = core qml testlib

CONFIG += c++11 console testcase
CONFIG -= app_bundle

TARGET = test
TEMPLATE = app

UIPATH = ../../ui
INCLUDEPATH += $$UIPATH

SOURCES += \
    ../common/main.cpp \
    test.cpp

HEADERS += \
    test.h

# Windows
LIBS += -lfwpuclnt -lkernel32 -lpsapi -luser32 -luuid -lws2_32
