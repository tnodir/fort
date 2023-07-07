include($$PWD/Driver-include.pri)

SOURCES += \
    $$PWD/common/fortconf.c \
    $$PWD/common/fortlog.c \
    $$PWD/common/fortprov.c \
    $$PWD/common/fort_wildmatch.c

HEADERS += \
    $$PWD/common/common.h \
    $$PWD/common/common_types.h \
    $$PWD/common/fortconf.h \
    $$PWD/common/fortdef.h \
    $$PWD/common/fortioctl.h \
    $$PWD/common/fortlog.h \
    $$PWD/common/fortprov.h \
    $$PWD/common/fort_wildmatch.h
