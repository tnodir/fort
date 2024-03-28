CONFIG(release, debug|release) {
    DEFINES *= NDEBUG
} else {
    DEFINES += SQLITE_DEBUG SQLITE_ENABLE_API_ARMOR
}

DEFINES += _HAVE_SQLITE_CONFIG_H

SQLITE_DIR = $$PWD/../../../3rdparty/sqlite

INCLUDEPATH += $$SQLITE_DIR

SOURCES += \
    $$SQLITE_DIR/sqlite3.c \
    $$PWD/dbquery.cpp \
    $$PWD/dbutil.cpp \
    $$PWD/dbvar.cpp \
    $$PWD/sqlitedb.cpp \
    $$PWD/sqlitestmt.cpp

HEADERS += \
    $$SQLITE_DIR/sqlite.h \
    $$SQLITE_DIR/sqlite3.h \
    $$SQLITE_DIR/sqlite_cfg.h \
    $$PWD/dbquery.h \
    $$PWD/dbutil.h \
    $$PWD/dbvar.h \
    $$PWD/sqlitedb.h \
    $$PWD/sqlitestmt.h \
    $$PWD/sqlitetypes.h
