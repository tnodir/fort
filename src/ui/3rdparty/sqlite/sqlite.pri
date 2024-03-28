CONFIG(release, debug|release) {
    DEFINES *= NDEBUG
} else {
    DEFINES += SQLITE_DEBUG SQLITE_ENABLE_API_ARMOR
}

DEFINES += _HAVE_SQLITE_CONFIG_H

SQLITE_DIR = $$PWD/../../../3rdparty/sqlite

INCLUDEPATH += $$SQLITE_DIR

SOURCES += \
    $$PWD/dbquery.cpp \
    $$SQLITE_DIR/sqlite3.c \
    $$PWD/dbvar.cpp \
    $$PWD/sqlitedb.cpp \
    $$PWD/sqlitestmt.cpp

HEADERS += \
    $$PWD/dbquery.h \
    $$SQLITE_DIR/sqlite.h \
    $$SQLITE_DIR/sqlite3.h \
    $$SQLITE_DIR/sqlite_cfg.h \
    $$PWD/dbvar.h \
    $$PWD/sqlitedb.h \
    $$PWD/sqlitestmt.h \
    $$PWD/sqlitetypes.h
