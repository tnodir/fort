include(../common/Test.pri)

SOURCES += \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/log/logbuffer.cpp \
    $$UIPATH/log/logentry.cpp \
    $$UIPATH/log/logentryblocked.cpp \
    $$UIPATH/log/logentryprocnew.cpp \
    $$UIPATH/log/logentrystattraf.cpp \
    $$UIPATH/util/dateutil.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/net/netutil.cpp \
    $$UIPATH/util/osutil.cpp \
    $$UIPATH/util/processinfo.cpp \
    $$UIPATH/util/stringutil.cpp

HEADERS += \
    $$UIPATH/fortcommon.h \
    $$UIPATH/fortcompat.h \
    $$UIPATH/log/logbuffer.h \
    $$UIPATH/log/logentry.h \
    $$UIPATH/log/logentryblocked.h \
    $$UIPATH/log/logentryprocnew.h \
    $$UIPATH/log/logentrystattraf.h \
    $$UIPATH/util/dateutil.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/netutil.h \
    $$UIPATH/util/osutil.h \
    $$UIPATH/util/processinfo.h \
    $$UIPATH/util/stringutil.h
