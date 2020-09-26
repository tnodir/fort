
QT += testlib

CONFIG += c++11 console
CONFIG -= app_bundle

TEMPLATE = app

include($$PWD/Common-include.pri)

# Driver integration
include($$UI_PWD/../driver/Driver-include.pri)

# GoogleTest
include($$PWD/GoogleTest-include.pri)

# 3rd party integrations
include($$UI_PWD/3rdparty/3rdparty-include.pri)

# Link to a static library
LIBS *= -L$$OUT_PWD/../Common
LIBS += -lCommonLib
