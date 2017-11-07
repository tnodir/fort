include(../common/Test.pri)

SOURCES += \
    $$UIPATH/conf/addressgroup.cpp \
    $$UIPATH/conf/appgroup.cpp \
    $$UIPATH/conf/firewallconf.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/log/logbuffer.cpp \
    $$UIPATH/log/logentry.cpp \
    $$UIPATH/log/logentryblocked.cpp \
    $$UIPATH/util/confutil.cpp \
    $$UIPATH/util/device.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/net/ip4range.cpp \
    $$UIPATH/util/net/netutil.cpp \
    $$UIPATH/util/osutil.cpp \
    $$UIPATH/util/processinfo.cpp

HEADERS += \
    $$UIPATH/conf/addressgroup.h \
    $$UIPATH/conf/appgroup.h \
    $$UIPATH/conf/firewallconf.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/log/logbuffer.h \
    $$UIPATH/log/logentry.h \
    $$UIPATH/log/logentryblocked.h \
    $$UIPATH/util/confutil.h \
    $$UIPATH/util/device.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/ip4range.h \
    $$UIPATH/util/net/netutil.h \
    $$UIPATH/util/osutil.h \
    $$UIPATH/util/processinfo.h
