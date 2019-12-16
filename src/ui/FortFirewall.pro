QT += core gui widgets

CONFIG += c++11

TEMPLATE = app
TARGET = FortFirewall
DESTDIR = ./
MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .rcc

SOURCES += \
    conf/addressgroup.cpp \
    conf/appgroup.cpp \
    conf/confmanager.cpp \
    conf/firewallconf.cpp \
    control/controlmanager.cpp \
    control/controlworker.cpp \
    driver/drivermanager.cpp \
    driver/driverworker.cpp \
    form/optionswindow.cpp \
    fortcommon.cpp \
    fortmanager.cpp \
    fortsettings.cpp \
    graph/axistickerspeed.cpp \
    graph/graphplot.cpp \
    graph/graphwindow.cpp \
    log/logbuffer.cpp \
    log/logentry.cpp \
    log/logentryblocked.cpp \
    log/logentryheartbeat.cpp \
    log/logentryprocnew.cpp \
    log/logentrystattraf.cpp \
    log/logmanager.cpp \
    log/model/appblockedmodel.cpp \
    log/model/appstatmodel.cpp \
    log/model/iplistmodel.cpp \
    log/model/stringlistmodel.cpp \
    log/model/traflistmodel.cpp \
    main.cpp \
    mainwindow.cpp \
    stat/quotamanager.cpp \
    stat/statmanager.cpp \
    stat/statsql.cpp \
    task/taskdownloader.cpp \
    task/taskinfo.cpp \
    task/taskinfotasix.cpp \
    task/taskinfoupdatechecker.cpp \
    task/taskmanager.cpp \
    task/tasktasix.cpp \
    task/taskupdatechecker.cpp \
    task/taskworker.cpp \
    translationmanager.cpp \
    util/app/appinfo.cpp \
    util/app/appinfocache.cpp \
    util/app/appinfojob.cpp \
    util/app/appinfomanager.cpp \
    util/app/appinfoworker.cpp \
    util/app/apputil.cpp \
    util/conf/addressrange.cpp \
    util/conf/confutil.cpp \
    util/dateutil.cpp \
    util/device.cpp \
    util/envmanager.cpp \
    util/fileutil.cpp \
    util/guiutil.cpp \
    util/hotkeymanager.cpp \
    util/logger.cpp \
    util/nativeeventfilter.cpp \
    util/net/hostinfo.cpp \
    util/net/hostinfocache.cpp \
    util/net/hostinfojob.cpp \
    util/net/hostinfomanager.cpp \
    util/net/ip4range.cpp \
    util/net/netdownloader.cpp \
    util/net/netutil.cpp \
    util/osutil.cpp \
    util/processinfo.cpp \
    util/stringutil.cpp \
    util/window/basewindowstatewatcher.cpp \
    util/window/widgetwindow.cpp \
    util/window/widgetwindowstatewatcher.cpp \
    util/window/windowstatewatcher.cpp \
    util/worker/workerjob.cpp \
    util/worker/workermanager.cpp \
    util/worker/workerobject.cpp

HEADERS += \
    conf/addressgroup.h \
    conf/appgroup.h \
    conf/confmanager.h \
    conf/firewallconf.h \
    control/controlmanager.h \
    control/controlworker.h \
    driver/drivermanager.h \
    driver/driverworker.h \
    form/optionswindow.h \
    fortcommon.h \
    fortmanager.h \
    fortsettings.h \
    graph/axistickerspeed.h \
    graph/graphplot.h \
    graph/graphwindow.h \
    log/logbuffer.h \
    log/logentry.h \
    log/logentryblocked.h \
    log/logentryheartbeat.h \
    log/logentryprocnew.h \
    log/logentrystattraf.h \
    log/logmanager.h \
    log/model/appblockedmodel.h \
    log/model/appstatmodel.h \
    log/model/iplistmodel.h \
    log/model/stringlistmodel.h \
    log/model/traflistmodel.h \
    mainwindow.h \
    stat/quotamanager.h \
    stat/statmanager.h \
    stat/statsql.h \
    task/taskdownloader.h \
    task/taskinfo.h \
    task/taskinfotasix.h \
    task/taskinfoupdatechecker.h \
    task/taskmanager.h \
    task/tasktasix.h \
    task/taskupdatechecker.h \
    task/taskworker.h \
    translationmanager.h \
    util/app/appinfo.h \
    util/app/appinfocache.h \
    util/app/appinfojob.h \
    util/app/appinfomanager.h \
    util/app/appinfoworker.h \
    util/app/apputil.h \
    util/conf/addressrange.h \
    util/conf/confutil.h \
    util/dateutil.h \
    util/device.h \
    util/envmanager.h \
    util/fileutil.h \
    util/guiutil.h \
    util/hotkeymanager.h \
    util/logger.h \
    util/nativeeventfilter.h \
    util/net/hostinfo.h \
    util/net/hostinfocache.h \
    util/net/hostinfojob.h \
    util/net/hostinfomanager.h \
    util/net/ip4range.h \
    util/net/netdownloader.h \
    util/net/netutil.h \
    util/osutil.h \
    util/processinfo.h \
    util/stringutil.h \
    util/window/basewindowstatewatcher.h \
    util/window/widgetwindow.h \
    util/window/widgetwindowstatewatcher.h \
    util/window/windowstatewatcher.h \
    util/worker/workerjob.h \
    util/worker/workermanager.h \
    util/worker/workerobject.h

include(../common/Common.pri)

QML_FILES += \
    qml/*.qml \
    qml/box/*.qml \
    qml/controls/*.qml \
    qml/pages/*.qml \
    qml/pages/addresses/*.qml \
    qml/pages/apps/*.qml \
    qml/pages/log/*.qml \
    qml/pages/schedule/*.qml

OTHER_FILES += \
    $${QML_FILES}

# Images
RESOURCES += fort_images.qrc

# Database Migrations
OTHER_FILES += \
    conf/migrations/*.sql \
    stat/migrations/*.sql \
    util/app/migrations/*.sql

RESOURCES += \
    conf/conf-migrations.qrc \
    stat/stat-migrations.qrc \
    util/app/app-migrations.qrc

# Shadow Build: Copy i18n/ to build path
!equals(PWD, $${OUT_PWD}) {
    i18n.files = $$files(i18n/*.qm)
    i18n.path = $${OUT_PWD}/i18n
    COPIES += i18n
}

# Windows
LIBS += -lfwpuclnt -lkernel32 -luser32 -luuid -lversion -lws2_32
RC_FILE = FortFirewall.rc
OTHER_FILES += $${RC_FILE}

# Kernel Driver
installer_build {
    BUILDCMD = MSBuild $$PWD/../driver/fortdrv.vcxproj /p:OutDir=./;IntDir=$$OUT_PWD/driver/

    fortdrv32.target = $$PWD/../driver/fortfw32.sys
    fortdrv32.commands = $$BUILDCMD /p:Platform=Win32

    fortdrv64.target = $$PWD/../driver/fortfw64.sys
    fortdrv64.commands = $$BUILDCMD /p:Platform=x64

    QMAKE_EXTRA_TARGETS += fortdrv32 fortdrv64
    PRE_TARGETDEPS += $$fortdrv32.target $$fortdrv64.target
}

# 3rd party integrations
CONFIG += qcustomplot sqlite
include(3rdparty/3rdparty.pri)
