lessThan(QT_VERSION, 5.7.1): error(This project requires Qt 5.7.1 or later)

QT += core gui qml widgets

CONFIG += c++11

TARGET = FortFirewall
TEMPLATE = app

SOURCES += \
    main.cpp \
    conf/appgroup.cpp \
    conf/firewallconf.cpp \
    firewallLog/logbuffer.cpp \
    firewallLog/logentry.cpp \
    fortcommon.cpp \
    util/confutil.cpp \
    util/fileutil.cpp \
    util/ip4range.cpp \
    util/netutil.cpp \
    util/processinfo.cpp

HEADERS += \
    conf/appgroup.h \
    conf/firewallconf.h \
    firewallLog/logbuffer.h \
    firewallLog/logentry.h \
    fortcommon.h \
    util/confutil.h \
    util/fileutil.h \
    util/ip4range.h \
    util/netutil.h \
    util/processinfo.h

QML_FILES += \
    qml/*.qml

OTHER_FILES += \
    $${QML_FILES} \
    *.ini

TRANSLATIONS += \
    i18n/i18n_ru.ts

# Compiled translation files
RESOURCES += fort_i18n.qrc

# Default FortFirewall.ini
RESOURCES += fort_ini.qrc

# Windows
LIBS += -lfwpuclnt -lkernel32 -lpsapi -luser32 -luuid -lws2_32
RC_FILE = fort.rc

# Kernel Driver
{
    BUILDCMD = MSBuild $$PWD/../driver/fortdrv.vcxproj /p:OutDir=./;IntDir=$$OUT_PWD/driver/

    fortdrv32.target = $$PWD/../driver/fortfw32.sys
    fortdrv32.commands = $$BUILDCMD /p:Platform=Win32

    fortdrv64.target = $$PWD/../driver/fortfw64.sys
    fortdrv64.commands = $$BUILDCMD /p:Platform=x64

    QMAKE_EXTRA_TARGETS += fortdrv32 fortdrv64
    PRE_TARGETDEPS += $$fortdrv32.target $$fortdrv64.target
}
