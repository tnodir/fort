QT -= gui
QT += network

CONFIG += c++2a

INCLUDEPATH *= $$PWD

# ETW and the Dnscache
LIBS *= -ladvapi32 -ldnsapi -ltdh

# Driver integration
include($$PWD/../driver/Driver-include.pri)

# 3rd party integrations
include($$PWD/3rdparty/c-ares-include.pri)
