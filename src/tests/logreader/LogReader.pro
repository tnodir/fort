include(../common/Test.pri)

SOURCES += \
    $$UIPATH/conf/appgroup.cpp \
    $$UIPATH/conf/firewallconf.cpp \
    $$UIPATH/activityLog/logbuffer.cpp \
    $$UIPATH/activityLog/logentry.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/util/confutil.cpp \
    $$UIPATH/util/device.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/ip4range.cpp \
    $$UIPATH/util/netutil.cpp \
    $$UIPATH/util/processinfo.cpp

HEADERS += \
    $$UIPATH/conf/appgroup.h \
    $$UIPATH/conf/firewallconf.h \
    $$UIPATH/activityLog/logbuffer.h \
    $$UIPATH/activityLog/logentry.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/util/confutil.h \
    $$UIPATH/util/device.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/ip4range.h \
    $$UIPATH/util/netutil.h \
    $$UIPATH/util/processinfo.h
