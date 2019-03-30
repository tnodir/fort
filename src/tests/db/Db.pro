include(../common/Test.pri)

QT += gui

SOURCES += \
    $$UIPATH/conf/addressgroup.cpp \
    $$UIPATH/conf/appgroup.cpp \
    $$UIPATH/conf/firewallconf.cpp \
    $$UIPATH/db/databasemanager.cpp \
    $$UIPATH/db/databasesql.cpp \
    $$UIPATH/db/quotamanager.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/fortsettings.cpp \
    $$UIPATH/util/dateutil.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/net/netutil.cpp

HEADERS += \
    $$UIPATH/conf/addressgroup.h \
    $$UIPATH/conf/appgroup.h \
    $$UIPATH/conf/firewallconf.h \
    $$UIPATH/db/databasemanager.h \
    $$UIPATH/db/databasesql.h \
    $$UIPATH/db/quotamanager.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/fortsettings.h \
    $$UIPATH/util/dateutil.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/netutil.h

# Database Migrations
RESOURCES += $$UIPATH/db/migrations.qrc

# 3rd party integrations
CONFIG += sqlite
include($$UIPATH/3rdparty/3rdparty.pri)
