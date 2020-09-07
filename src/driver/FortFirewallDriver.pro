include(Driver.pri)

QT = core

SOURCES += \
    dummy.c \
    fortbuf.c \
    fortcnf.c \
    fortdrv.c \
    fortpkt.c \
    fortstat.c \
    forttds.c \
    forttlsf.c \
    forttmr.c \
    fortwrk.c

HEADERS += \
    fortbuf.h \
    fortcnf.h \
    fortdrv.h \
    fortpkt.h \
    fortstat.h \
    forttds.h \
    forttlsf.h \
    forttmr.h \
    fortwrk.h

OTHER_FILES += \
    scripts/*.bat

# Kernel Driver
{
    OUT_DIR = $$PWD/../../deploy/build/driver
    BUILDCMD = $$PWD/msvcbuild.bat

    fortdrv32.target = $$OUT_DIR/fortfw32.sys
    fortdrv32.commands = $$BUILDCMD Win32

    fortdrv64.target = $$OUT_DIR/fortfw64.sys
    fortdrv64.commands = $$BUILDCMD x64

    QMAKE_EXTRA_TARGETS += fortdrv32 fortdrv64
    PRE_TARGETDEPS += $$fortdrv32.target $$fortdrv64.target
}
