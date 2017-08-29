include(../common/Test.pri)

SOURCES += \
    $$UIPATH/conf/appgroup.cpp \
    $$UIPATH/conf/firewallconf.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/util/confutil.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/ip4range.cpp \
    $$UIPATH/util/netutil.cpp

HEADERS += \
    $$UIPATH/conf/appgroup.h \
    $$UIPATH/conf/firewallconf.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/util/confutil.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/ip4range.h \
    $$UIPATH/util/netutil.h
