include(../global.pri)

include(Driver.pri)

QT = core

SOURCES += \
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
    proxycb/fortpcb_drv.c \
    proxycb/fortpcb_dst.c \
    proxycb/fortpcb_src.c \
    test/main.c \
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

ASM_FILES += \
    proxycb/fortpcb_dst_x86.asm \
    proxycb/fortpcb_src_x86.asm

OTHER_FILES += \
    loader/fort.rsa.pub \
    scripts/*.bat

# Windows
LIBS *= -lntdll

# MASM
!no_masm {
    contains(QMAKE_TARGET.arch, x86_64): MASM_EXE = ml64
    else: MASM_EXE = ml

    masm.input = ASM_FILES
    masm.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_IN_BASE}$${first(QMAKE_EXT_OBJ)}
    masm.variable_out = OBJECTS
    masm.commands = $$MASM_EXE /c /nologo /Zi /Fo ${QMAKE_FILE_OUT} /W3 /Ta ${QMAKE_FILE_NAME}

    QMAKE_EXTRA_COMPILERS += masm
} else {
    SOURCES += \
        proxycb/fortpcb_dst_dummy.c \
        proxycb/fortpcb_src_dummy.c
}
