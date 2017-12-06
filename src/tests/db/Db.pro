include(../common/Test.pri)

SOURCES += \
    $$UIPATH/db/databasemanager.cpp \
    $$UIPATH/db/databasesql.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/util/dateutil.cpp \
    $$UIPATH/util/fileutil.cpp

HEADERS += \
    $$UIPATH/db/databasemanager.h \
    $$UIPATH/db/databasesql.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/util/dateutil.h \
    $$UIPATH/util/fileutil.h

include($$UIPATH/db/sqlite/sqlite.pri)
