include(../common/Test.pri)

SOURCES += \
    $$UIPATH/conf/addressgroup.cpp \
    $$UIPATH/conf/appgroup.cpp \
    $$UIPATH/conf/firewallconf.cpp \
    $$UIPATH/db/databasemanager.cpp \
    $$UIPATH/db/databasesql.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/util/dateutil.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/net/netutil.cpp

HEADERS += \
    $$UIPATH/conf/addressgroup.h \
    $$UIPATH/conf/appgroup.h \
    $$UIPATH/conf/firewallconf.h \
    $$UIPATH/db/databasemanager.h \
    $$UIPATH/db/databasesql.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/util/dateutil.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/netutil.h

include($$UIPATH/db/sqlite/sqlite.pri)
