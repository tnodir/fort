include(../Common/Common.pri)

HEADERS += \
    tst_logbuffer.h \

SOURCES += \
    tst_main.cpp

SOURCES += \
    $$UI_PWD/fortcommon.cpp \
    $$UI_PWD/log/logbuffer.cpp \
    $$UI_PWD/log/logentry.cpp \
    $$UI_PWD/log/logentryblocked.cpp \
    $$UI_PWD/log/logentryprocnew.cpp \
    $$UI_PWD/log/logentrystattraf.cpp \
    $$UI_PWD/util/dateutil.cpp \
    $$UI_PWD/util/fileutil.cpp \
    $$UI_PWD/util/net/netutil.cpp \
    $$UI_PWD/util/osutil.cpp \
    $$UI_PWD/util/processinfo.cpp \
    $$UI_PWD/util/stringutil.cpp

HEADERS += \
    $$UI_PWD/fortcommon.h \
    $$UI_PWD/fortcompat.h \
    $$UI_PWD/log/logbuffer.h \
    $$UI_PWD/log/logentry.h \
    $$UI_PWD/log/logentryblocked.h \
    $$UI_PWD/log/logentryprocnew.h \
    $$UI_PWD/log/logentrystattraf.h \
    $$UI_PWD/util/dateutil.h \
    $$UI_PWD/util/fileutil.h \
    $$UI_PWD/util/net/netutil.h \
    $$UI_PWD/util/osutil.h \
    $$UI_PWD/util/processinfo.h \
    $$UI_PWD/util/stringutil.h
