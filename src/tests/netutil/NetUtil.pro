include(../common/Test.pri)

SOURCES += \
    $$UIPATH/conf/addressgroup.cpp \
    $$UIPATH/conf/appgroup.cpp \
    $$UIPATH/conf/firewallconf.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/task/taskdownloader.cpp \
    $$UIPATH/task/taskzonedownloader.cpp \
    $$UIPATH/task/taskworker.cpp \
    $$UIPATH/util/conf/addressrange.cpp \
    $$UIPATH/util/conf/confutil.cpp \
    $$UIPATH/util/dateutil.cpp \
    $$UIPATH/util/envmanager.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/net/ip4range.cpp \
    $$UIPATH/util/net/netdownloader.cpp \
    $$UIPATH/util/net/netutil.cpp \
    $$UIPATH/util/osutil.cpp \
    $$UIPATH/util/processinfo.cpp \
    $$UIPATH/util/stringutil.cpp

HEADERS += \
    $$UIPATH/conf/addressgroup.h \
    $$UIPATH/conf/appgroup.h \
    $$UIPATH/conf/firewallconf.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/fortcompat.h \
    $$UIPATH/task/taskdownloader.h \
    $$UIPATH/task/taskzonedownloader.h \
    $$UIPATH/task/taskworker.h \
    $$UIPATH/util/conf/addressrange.h \
    $$UIPATH/util/conf/confutil.h \
    $$UIPATH/util/dateutil.h \
    $$UIPATH/util/envmanager.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/ip4range.h \
    $$UIPATH/util/net/netdownloader.h \
    $$UIPATH/util/net/netutil.h \
    $$UIPATH/util/osutil.h \
    $$UIPATH/util/processinfo.h \
    $$UIPATH/util/stringutil.h

DEFINES += \
    PWD='"$$PWD"'
