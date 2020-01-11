QT = core testlib

CONFIG += c++11 console testcase
CONFIG -= app_bundle

TARGET = test
TEMPLATE = app

UIPATH = $$PWD/../../ui
INCLUDEPATH += $$UIPATH

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/main.cpp \
    test.cpp

HEADERS += \
    $$PWD/commontest.h \
    test.h

# Windows
LIBS += -lfwpuclnt -lkernel32 -luser32 -luuid -lws2_32
