INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/common/fortconf.c \
    $$PWD/common/fortlog.c \
    $$PWD/common/fortprov.c \
    $$PWD/common/wildmatch.c

HEADERS += \
    $$PWD/common/common.h \
    $$PWD/common/fortconf.h \
    $$PWD/common/fortdev.h \
    $$PWD/common/fortlog.h \
    $$PWD/common/fortprov.h \
    $$PWD/common/wildmatch.h

# Windows
LIBS *= -lfwpuclnt -lkernel32 -luser32 -luuid -lversion -lws2_32
