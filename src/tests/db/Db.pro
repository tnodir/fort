include(../common/Test.pri)

SOURCES += \
    $$UIPATH/db/databasemanager.cpp \
    $$UIPATH/fortcommon.cpp \
    $$UIPATH/util/fileutil.cpp

HEADERS += \
    $$UIPATH/db/databasemanager.h \
    $$UIPATH/fortcommon.h \
    $$UIPATH/util/fileutil.h

include($$UIPATH/db/sqlite/sqlite.pri)
