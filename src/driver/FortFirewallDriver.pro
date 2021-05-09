include(../global.pri)

include(Driver.pri)

QT = core

SOURCES += \
    dummy.c \
    fortbuf.c \
    fortcb.c \
    fortcnf.c \
    fortcout.c \
    fortdev.c \
    fortdrv.c \
    fortpkt.c \
    fortstat.c \
    forttds.c \
    forttlsf.c \
    forttmr.c \
    fortwrk.c \
    wdm/um_fwpmk.c \
    wdm/um_fwpsk.c \
    wdm/um_ndis.c \
    wdm/um_wdm.c

HEADERS += \
    fortbuf.h \
    fortcb.h \
    fortcnf.h \
    fortcout.h \
    fortdev.h \
    fortdrv.h \
    fortpkt.h \
    fortstat.h \
    forttds.h \
    forttlsf.h \
    forttmr.h \
    fortwrk.h \
    wdm/um_fwpmk.h \
    wdm/um_fwpsk.h \
    wdm/um_ndis.h \
    wdm/um_wdm.h

OTHER_FILES += \
    scripts/*.bat

# Windows
LIBS *= -lntdll

# Kernel Driver
{
    BUILDCMD = $$PWD/msvcbuild.bat

    #QMAKE_POST_LINK += $$BUILDCMD Win32
    QMAKE_POST_LINK += $$BUILDCMD x64
}
