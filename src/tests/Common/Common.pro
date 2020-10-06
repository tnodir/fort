
QT += gui

CONFIG += c++17 console
CONFIG -= app_bundle
CONFIG -= debug_and_release

TARGET = CommonLib
CONFIG += staticlib
TEMPLATE = lib

include($$PWD/Common-include.pri)

SOURCES += \
    $$UI_PWD/conf/addressgroup.cpp \
    $$UI_PWD/conf/appgroup.cpp \
    $$UI_PWD/conf/firewallconf.cpp \
    $$UI_PWD/fortcommon.cpp \
    $$UI_PWD/fortsettings.cpp \
    $$UI_PWD/log/logbuffer.cpp \
    $$UI_PWD/log/logentry.cpp \
    $$UI_PWD/log/logentryblocked.cpp \
    $$UI_PWD/log/logentryprocnew.cpp \
    $$UI_PWD/log/logentrystattraf.cpp \
    $$UI_PWD/stat/quotamanager.cpp \
    $$UI_PWD/stat/statmanager.cpp \
    $$UI_PWD/stat/statsql.cpp \
    $$UI_PWD/task/taskdownloader.cpp \
    $$UI_PWD/task/taskzonedownloader.cpp \
    $$UI_PWD/task/taskworker.cpp \
    $$UI_PWD/util/conf/addressrange.cpp \
    $$UI_PWD/util/conf/confutil.cpp \
    $$UI_PWD/util/dateutil.cpp \
    $$UI_PWD/util/device.cpp \
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
    $$UI_PWD/fortsettings.h \
    $$UI_PWD/log/logbuffer.h \
    $$UI_PWD/log/logentry.h \
    $$UI_PWD/log/logentryblocked.h \
    $$UI_PWD/log/logentryprocnew.h \
    $$UI_PWD/log/logentrystattraf.h \
    $$UI_PWD/stat/quotamanager.h \
    $$UI_PWD/stat/statmanager.h \
    $$UI_PWD/stat/statsql.h \
    $$UI_PWD/task/taskdownloader.h \
    $$UI_PWD/task/taskzonedownloader.h \
    $$UI_PWD/task/taskworker.h \
    $$UI_PWD/util/conf/addressrange.h \
    $$UI_PWD/util/conf/confappswalker.h \
    $$UI_PWD/util/conf/confutil.h \
    $$UI_PWD/util/dateutil.h \
    $$UI_PWD/util/device.h \
    $$UI_PWD/util/envmanager.h \
    $$UI_PWD/util/fileutil.h \
    $$UI_PWD/util/net/ip4range.h \
    $$UI_PWD/util/net/netdownloader.h \
    $$UI_PWD/util/net/netutil.h \
    $$UI_PWD/util/osutil.h \
    $$UI_PWD/util/processinfo.h \
    $$UI_PWD/util/stringutil.h

# Driver integration
include($$UI_PWD/../driver/Driver.pri)

# GoogleTest
include($$PWD/GoogleTest.pri)

# 3rd party integrations
CONFIG += sqlite
include($$UI_PWD/3rdparty/3rdparty.pri)
