QT = core testlib

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
