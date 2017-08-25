lessThan(QT_VERSION, 5.7.1): error(This project requires Qt 5.7.1 or later)

QT += core gui qml widgets

CONFIG += c++11

TARGET = FortFirewall
TEMPLATE = app

SOURCES += \
    main.cpp \
    fortmanager.cpp

HEADERS += \
    fortmanager.h

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

# Windows RC
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
