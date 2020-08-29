include(../common/Test.pri)

QT += gui

SOURCES += \
    $$UIPATH/conf/addressgroup.cpp \
    $$UIPATH/conf/appgroup.cpp \
    $$UIPATH/conf/firewallconf.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/fortsettings.cpp \
    $$UIPATH/stat/quotamanager.cpp \
    $$UIPATH/stat/statmanager.cpp \
    $$UIPATH/stat/statsql.cpp \
    $$UIPATH/util/dateutil.cpp \
    $$UIPATH/util/fileutil.cpp \
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
    $$UIPATH/fortsettings.h \
    $$UIPATH/stat/quotamanager.h \
    $$UIPATH/stat/statmanager.h \
    $$UIPATH/stat/statsql.h \
    $$UIPATH/util/dateutil.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/netutil.h \
    $$UIPATH/util/osutil.h \
    $$UIPATH/util/processinfo.h \
    $$UIPATH/util/stringutil.h

# Test Data
RESOURCES += data.qrc

# Stat Migrations
RESOURCES += $$UIPATH/stat/stat-migrations.qrc

# 3rd party integrations
CONFIG += sqlite
include($$UIPATH/3rdparty/3rdparty.pri)
