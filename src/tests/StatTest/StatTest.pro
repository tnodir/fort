include(../Common/Common.pri)

QT += gui

HEADERS += \
    tst_stat.h

SOURCES += \
    tst_main.cpp

SOURCES += \
    $$UI_PWD/conf/addressgroup.cpp \
    $$UI_PWD/conf/appgroup.cpp \
    $$UI_PWD/conf/firewallconf.cpp \
    $$UI_PWD/fortcommon.cpp \
    $$UI_PWD/fortsettings.cpp \
    $$UI_PWD/stat/quotamanager.cpp \
    $$UI_PWD/stat/statmanager.cpp \
    $$UI_PWD/stat/statsql.cpp \
    $$UI_PWD/util/dateutil.cpp \
    $$UI_PWD/util/fileutil.cpp \
    $$UI_PWD/util/net/netutil.cpp \
    $$UI_PWD/util/osutil.cpp \
    $$UI_PWD/util/processinfo.cpp \
    $$UI_PWD/util/stringutil.cpp

HEADERS += \
    $$UI_PWD/conf/addressgroup.h \
    $$UI_PWD/conf/appgroup.h \
    $$UI_PWD/conf/firewallconf.h \
    $$UI_PWD/fortcommon.h \
    $$UI_PWD/fortcompat.h \
    $$UI_PWD/fortsettings.h \
    $$UI_PWD/stat/quotamanager.h \
    $$UI_PWD/stat/statmanager.h \
    $$UI_PWD/stat/statsql.h \
    $$UI_PWD/util/dateutil.h \
    $$UI_PWD/util/fileutil.h \
    $$UI_PWD/util/net/netutil.h \
    $$UI_PWD/util/osutil.h \
    $$UI_PWD/util/processinfo.h \
    $$UI_PWD/util/stringutil.h

# Test Data
RESOURCES += data.qrc

# Stat Migrations
RESOURCES += $$UI_PWD/stat/stat-migrations.qrc

# 3rd party integrations
CONFIG += sqlite
include($$UI_PWD/3rdparty/3rdparty.pri)
