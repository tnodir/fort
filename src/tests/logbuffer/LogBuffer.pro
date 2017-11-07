include(../common/Test.pri)

SOURCES += \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/log/logbuffer.cpp \
    $$UIPATH/log/logentry.cpp \
    $$UIPATH/log/logentryblocked.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/net/netutil.cpp \
    $$UIPATH/util/osutil.cpp \
    $$UIPATH/util/processinfo.cpp

HEADERS += \
    $$UIPATH/fortcommon.h \
    $$UIPATH/log/logbuffer.h \
    $$UIPATH/log/logentry.h \
    $$UIPATH/log/logentryblocked.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/netutil.h \
    $$UIPATH/util/osutil.h \
    $$UIPATH/util/processinfo.h
