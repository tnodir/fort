include(../common/Test.pri)

SOURCES += \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/log/logbuffer.cpp \
    $$UIPATH/log/logentry.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/netutil.cpp \
    $$UIPATH/util/osutil.cpp \
    $$UIPATH/util/processinfo.cpp

HEADERS += \
    $$UIPATH/fortcommon.h \
    $$UIPATH/log/logbuffer.h \
    $$UIPATH/log/logentry.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/netutil.h \
    $$UIPATH/util/osutil.h \
    $$UIPATH/util/processinfo.h
