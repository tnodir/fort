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
    fortscb.c \
    fortstat.c \
    forttds.c \
    forttlsf.c \
    forttmr.c \
    fortutl.c \
    fortwrk.c \
    loader/fortdl.c \
    loader/fortimg.c \
    loader/fortmm.c \
    proxycb/fortpcb_def.c \
    proxycb/fortpcb_drv.c \
    proxycb/fortpcb_dst.c \
    proxycb/fortpcb_src.c \
    wdm/um_aux_klib.c \
    wdm/um_fwpmk.c \
    wdm/um_fwpsk.c \
    wdm/um_ndis.c \
    wdm/um_ntddk.c \
    wdm/um_wdm.c

HEADERS += \
    fortbuf.h \
    fortcb.h \
    fortcnf.h \
    fortcout.h \
    fortdev.h \
    fortdrv.h \
    fortpkt.h \
    fortscb.h \
    fortstat.h \
    forttds.h \
    forttlsf.h \
    forttmr.h \
    fortutl.h \
    fortwrk.h \
    loader/fortdl.h \
    loader/fortimg.h \
    loader/fortmm.h \
    proxycb/fortpcb_def.h \
    proxycb/fortpcb_drv.h \
    proxycb/fortpcb_dst.h \
    proxycb/fortpcb_src.h \
    wdm/um_aux_klib.h \
    wdm/um_fwpmk.h \
    wdm/um_fwpsk.h \
    wdm/um_ndis.h \
    wdm/um_ntddk.h \
    wdm/um_wdm.h

OTHER_FILES += \
    loader/fort.rsa.pub \
    scripts/*.bat

# Windows
LIBS *= -lntdll

# Kernel Driver
{
    BUILDCMD = $$PWD/msvcbuild.bat

    #QMAKE_POST_LINK += $$BUILDCMD Win32
    #QMAKE_POST_LINK += $$BUILDCMD x64
}
