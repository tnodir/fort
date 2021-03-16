include($$PWD/Driver-include.pri)

SOURCES += \
    $$PWD/common/fortconf.c \
    $$PWD/common/fortlog.c \
    $$PWD/common/fortprov.c \
    $$PWD/common/wildmatch.c

HEADERS += \
    $$PWD/common/common.h \
    $$PWD/common/fortconf.h \
    $$PWD/common/fortdef.h \
    $$PWD/common/fortlog.h \
    $$PWD/common/fortprov.h \
    $$PWD/common/wildmatch.h
