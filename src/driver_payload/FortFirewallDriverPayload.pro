include(../global.pri)

include(../ui/FortFirewallUI.pri)

CONFIG += console
CONFIG -= debug_and_release

TARGET = DriverPayload
TEMPLATE = app

SOURCES += \
    driverpayload.cpp \
    main.cpp

HEADERS += \
    driverpayload.h

# Windows
LIBS *= -limagehlp
