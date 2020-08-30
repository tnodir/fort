include($$PWD/GoogleTest.pri)

QT = core testlib

CONFIG += c++11 console
CONFIG -= app_bundle

TEMPLATE = app

UI_PWD = $$PWD/../../ui

# Driver integration
include($$UI_PWD/../driver/Driver.pri)

# Windows
LIBS += -lfwpuclnt -lkernel32 -luser32 -luuid -lws2_32

INCLUDEPATH += $$PWD
INCLUDEPATH += $$UI_PWD
