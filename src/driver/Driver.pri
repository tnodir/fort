INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/common/fortconf.c \
    $$PWD/common/fortlog.c \
    $$PWD/common/fortprov.c

HEADERS += \
    $$PWD/common/common.h \
    $$PWD/common/fortconf.h \
    $$PWD/common/fortdev.h

# Kernel Driver
installer_build {
    BUILDCMD = MSBuild $$PWD/fortdrv.vcxproj /p:OutDir=./;IntDir=$$OUT_PWD/driver/

    fortdrv32.target = $$PWD/fortfw32.sys
    fortdrv32.commands = $$BUILDCMD /p:Platform=Win32

    fortdrv64.target = $$PWD/fortfw64.sys
    fortdrv64.commands = $$BUILDCMD /p:Platform=x64

    QMAKE_EXTRA_TARGETS += fortdrv32 fortdrv64
    PRE_TARGETDEPS += $$fortdrv32.target $$fortdrv64.target
}
