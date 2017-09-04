include(../common/Test.pri)

SOURCES += \
    $$UIPATH/activityLog/logbuffer.cpp \
    $$UIPATH/activityLog/logentry.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/netutil.cpp \
    $$UIPATH/util/processinfo.cpp

HEADERS += \
    $$UIPATH/activityLog/logbuffer.h \
    $$UIPATH/activityLog/logentry.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/netutil.h \
    $$UIPATH/util/processinfo.h
