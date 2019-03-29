include(../common/Test.pri)

SOURCES += \
    $$UIPATH/conf/addressgroup.cpp \
    $$UIPATH/conf/appgroup.cpp \
    $$UIPATH/conf/firewallconf.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/util/conf/addressrange.cpp \
    $$UIPATH/util/conf/confutil.cpp \
    $$UIPATH/util/dateutil.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/net/ip4range.cpp \
    $$UIPATH/util/net/netutil.cpp

HEADERS += \
    $$UIPATH/conf/addressgroup.h \
    $$UIPATH/conf/appgroup.h \
    $$UIPATH/conf/firewallconf.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/util/conf/addressrange.h \
    $$UIPATH/util/conf/confutil.h \
    $$UIPATH/util/dateutil.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/ip4range.h \
    $$UIPATH/util/net/netutil.h
