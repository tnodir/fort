include(../common/Test.pri)

SOURCES += \
    $$UIPATH/conf/addressgroup.cpp \
    $$UIPATH/conf/appgroup.cpp \
    $$UIPATH/conf/firewallconf.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/log/logbuffer.cpp \
    $$UIPATH/log/logentry.cpp \
    $$UIPATH/log/logentryblocked.cpp \
    $$UIPATH/log/logentryprocnew.cpp \
    $$UIPATH/log/logentrystattraf.cpp \
    $$UIPATH/util/conf/addressrange.cpp \
    $$UIPATH/util/conf/confutil.cpp \
    $$UIPATH/util/dateutil.cpp \
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
    $$UIPATH/log/logentryprocnew.h \
    $$UIPATH/log/logentrystattraf.h \
    $$UIPATH/util/conf/addressrange.h \
    $$UIPATH/util/conf/confutil.h \
    $$UIPATH/util/dateutil.h \
    $$UIPATH/util/device.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/ip4range.h \
    $$UIPATH/util/net/netutil.h \
    $$UIPATH/util/osutil.h \
    $$UIPATH/util/processinfo.h
