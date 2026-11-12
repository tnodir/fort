QT -= gui
QT += network

CONFIG += c++2a

INCLUDEPATH *= $$PWD

# Driver integration
include($$PWD/../driver/Driver-include.pri)

# 3rd party integrations
include($$PWD/3rdparty/c-ares-include.pri)
