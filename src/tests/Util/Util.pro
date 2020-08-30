include(../Common/Common.pri)

HEADERS += \
    tst_confutil.h \
    tst_fileutil.h \
    tst_netutil.h

SOURCES += \
    tst_main.cpp

SOURCES += \
    $$UI_PWD/conf/addressgroup.cpp \
    $$UI_PWD/conf/appgroup.cpp \
    $$UI_PWD/conf/firewallconf.cpp \
    $$UI_PWD/fortcommon.cpp \
    $$UI_PWD/task/taskdownloader.cpp \
    $$UI_PWD/task/taskzonedownloader.cpp \
    $$UI_PWD/task/taskworker.cpp \
    $$UI_PWD/util/conf/addressrange.cpp \
    $$UI_PWD/util/conf/confutil.cpp \
    $$UI_PWD/util/dateutil.cpp \
    $$UI_PWD/util/envmanager.cpp \
    $$UI_PWD/util/fileutil.cpp \
    $$UI_PWD/util/net/ip4range.cpp \
    $$UI_PWD/util/net/netdownloader.cpp \
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
    $$UI_PWD/task/taskdownloader.h \
    $$UI_PWD/task/taskzonedownloader.h \
    $$UI_PWD/task/taskworker.h \
    $$UI_PWD/util/conf/addressrange.h \
    $$UI_PWD/util/conf/confappswalker.h \
    $$UI_PWD/util/conf/confutil.h \
    $$UI_PWD/util/dateutil.h \
    $$UI_PWD/util/envmanager.h \
    $$UI_PWD/util/fileutil.h \
    $$UI_PWD/util/net/ip4range.h \
    $$UI_PWD/util/net/netdownloader.h \
    $$UI_PWD/util/net/netutil.h \
    $$UI_PWD/util/osutil.h \
    $$UI_PWD/util/processinfo.h \
    $$UI_PWD/util/stringutil.h

# Test Data
RESOURCES += data.qrc
