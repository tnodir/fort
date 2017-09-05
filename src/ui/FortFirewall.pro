QT += core gui qml widgets

CONFIG += c++11

TARGET = FortFirewall
TEMPLATE = app

SOURCES += \
    main.cpp \
    activityLog/logbuffer.cpp \
    activityLog/logentry.cpp \
    conf/addressgroup.cpp \
    conf/appgroup.cpp \
    conf/firewallconf.cpp \
    driver/drivermanager.cpp \
    driver/driverworker.cpp \
    fortcommon.cpp \
    fortmanager.cpp \
    fortsettings.cpp \
    util/confutil.cpp \
    util/device.cpp \
    util/fileutil.cpp \
    util/ip4range.cpp \
    util/netutil.cpp \
    util/processinfo.cpp \
    util/osutil.cpp

HEADERS += \
    activityLog/logbuffer.h \
    activityLog/logentry.h \
    conf/addressgroup.h \
    conf/appgroup.h \
    conf/firewallconf.h \
    driver/drivermanager.h \
    driver/driverworker.h \
    fortcommon.h \
    fortmanager.h \
    fortsettings.h \
    util/confutil.h \
    util/device.h \
    util/fileutil.h \
    util/ip4range.h \
    util/netutil.h \
    util/processinfo.h \
    util/osutil.h

QML_FILES += \
    qml/*.qml \
    qml/controls/*.qml \
    qml/pages/*.qml \
    qml/pages/addresses/*.qml \
    qml/pages/apps/*.qml

OTHER_FILES += \
    $${QML_FILES} \
    *.ini

TRANSLATIONS += \
    i18n/i18n_ru.ts

# QML files
RESOURCES += fort_qml.qrc

# Compiled translation files
#RESOURCES += fort_i18n.qrc

# Default FortFirewall.ini
RESOURCES += fort_ini.qrc

# Images
RESOURCES += fort_images.qrc

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
