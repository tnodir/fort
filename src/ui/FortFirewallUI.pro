QT += core gui widgets

CONFIG += c++17

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
    form/conn/connectionscontroller.cpp \
    form/conn/connectionswindow.cpp \
    form/controls/checkspincombo.cpp \
    form/controls/checktimeperiod.cpp \
    form/controls/controlutil.cpp \
    form/controls/labelcolor.cpp \
    form/controls/labelspin.cpp \
    form/controls/labelspincombo.cpp \
    form/controls/listview.cpp \
    form/controls/mainwindow.cpp \
    form/controls/plaintextedit.cpp \
    form/controls/spincombo.cpp \
    form/controls/tabbar.cpp \
    form/controls/tableview.cpp \
    form/controls/textarea2splitter.cpp \
    form/controls/textarea2splitterhandle.cpp \
    form/graph/axistickerspeed.cpp \
    form/graph/graphplot.cpp \
    form/graph/graphwindow.cpp \
    form/opt/optionscontroller.cpp \
    form/opt/optionswindow.cpp \
    form/opt/pages/addresses/addressescolumn.cpp \
    form/opt/pages/addressespage.cpp \
    form/opt/pages/applicationspage.cpp \
    form/opt/pages/apps/appscolumn.cpp \
    form/opt/pages/basepage.cpp \
    form/opt/pages/mainpage.cpp \
    form/opt/pages/optionspage.cpp \
    form/opt/pages/schedulepage.cpp \
    form/opt/pages/statisticspage.cpp \
    form/prog/programscontroller.cpp \
    form/prog/programswindow.cpp \
    form/zone/zonescontroller.cpp \
    form/zone/zoneswindow.cpp \
    fortcommon.cpp \
    fortmanager.cpp \
    fortsettings.cpp \
    log/logbuffer.cpp \
    log/logentry.cpp \
    log/logentryblocked.cpp \
    log/logentryblockedip.cpp \
    log/logentryprocnew.cpp \
    log/logentrystattraf.cpp \
    log/logentrytime.cpp \
    log/logmanager.cpp \
    main.cpp \
    model/applistmodel.cpp \
    model/appstatmodel.cpp \
    model/connlistmodel.cpp \
    model/traflistmodel.cpp \
    model/zonelistmodel.cpp \
    model/zonesourcewrapper.cpp \
    model/zonetypewrapper.cpp \
    stat/quotamanager.cpp \
    stat/statmanager.cpp \
    stat/statsql.cpp \
    task/taskdownloader.cpp \
    task/taskinfo.cpp \
    task/taskinfoupdatechecker.cpp \
    task/taskinfozonedownloader.cpp \
    task/tasklistmodel.cpp \
    task/taskmanager.cpp \
    task/taskupdatechecker.cpp \
    task/taskworker.cpp \
    task/taskzonedownloader.cpp \
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
    util/iconcache.cpp \
    util/json/jsonutil.cpp \
    util/json/mapwrapper.cpp \
    util/logger.cpp \
    util/model/stringlistmodel.cpp \
    util/model/tableitemmodel.cpp \
    util/model/tablesqlmodel.cpp \
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
    util/textareautil.cpp \
    util/triggertimer.cpp \
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
    form/conn/connectionscontroller.h \
    form/conn/connectionswindow.h \
    form/controls/checkspincombo.h \
    form/controls/checktimeperiod.h \
    form/controls/controlutil.h \
    form/controls/labelcolor.h \
    form/controls/labelspin.h \
    form/controls/labelspincombo.h \
    form/controls/listview.h \
    form/controls/mainwindow.h \
    form/controls/plaintextedit.h \
    form/controls/spincombo.h \
    form/controls/tabbar.h \
    form/controls/tableview.h \
    form/controls/textarea2splitter.h \
    form/controls/textarea2splitterhandle.h \
    form/graph/axistickerspeed.h \
    form/graph/graphplot.h \
    form/graph/graphwindow.h \
    form/opt/optionscontroller.h \
    form/opt/optionswindow.h \
    form/opt/pages/addresses/addressescolumn.h \
    form/opt/pages/addressespage.h \
    form/opt/pages/applicationspage.h \
    form/opt/pages/apps/appscolumn.h \
    form/opt/pages/basepage.h \
    form/opt/pages/mainpage.h \
    form/opt/pages/optionspage.h \
    form/opt/pages/schedulepage.h \
    form/opt/pages/statisticspage.h \
    form/prog/programscontroller.h \
    form/prog/programswindow.h \
    form/zone/zonescontroller.h \
    form/zone/zoneswindow.h \
    fortcommon.h \
    fortcompat.h \
    fortmanager.h \
    fortsettings.h \
    log/logbuffer.h \
    log/logentry.h \
    log/logentryblocked.h \
    log/logentryblockedip.h \
    log/logentryprocnew.h \
    log/logentrystattraf.h \
    log/logentrytime.h \
    log/logmanager.h \
    model/applistmodel.h \
    model/appstatmodel.h \
    model/connlistmodel.h \
    model/traflistmodel.h \
    model/zonelistmodel.h \
    model/zonesourcewrapper.h \
    model/zonetypewrapper.h \
    stat/quotamanager.h \
    stat/statmanager.h \
    stat/statsql.h \
    task/taskdownloader.h \
    task/taskinfo.h \
    task/taskinfoupdatechecker.h \
    task/taskinfozonedownloader.h \
    task/tasklistmodel.h \
    task/taskmanager.h \
    task/taskupdatechecker.h \
    task/taskworker.h \
    task/taskzonedownloader.h \
    translationmanager.h \
    util/app/appinfo.h \
    util/app/appinfocache.h \
    util/app/appinfojob.h \
    util/app/appinfomanager.h \
    util/app/appinfoworker.h \
    util/app/apputil.h \
    util/conf/addressrange.h \
    util/conf/confappswalker.h \
    util/conf/confutil.h \
    util/dateutil.h \
    util/device.h \
    util/envmanager.h \
    util/fileutil.h \
    util/guiutil.h \
    util/hotkeymanager.h \
    util/iconcache.h \
    util/json/jsonutil.h \
    util/json/mapwrapper.h \
    util/logger.h \
    util/model/stringlistmodel.h \
    util/model/tableitemmodel.h \
    util/model/tablesqlmodel.h \
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
    util/textareautil.h \
    util/triggertimer.h \
    util/window/basewindowstatewatcher.h \
    util/window/widgetwindow.h \
    util/window/widgetwindowstatewatcher.h \
    util/window/windowstatewatcher.h \
    util/worker/workerjob.h \
    util/worker/workermanager.h \
    util/worker/workerobject.h

# Version
include(../version/Version.pri)

# Driver integration
include(../driver/Driver.pri)

# Icons & Images
RESOURCES += fort-icons.qrc fort-images.qrc

# Database Migrations
OTHER_FILES += \
    conf/migrations/*.sql \
    stat/migrations/*.sql \
    util/app/migrations/*.sql

RESOURCES += \
    conf/conf-migrations.qrc \
    stat/stat-migrations.qrc \
    util/app/app-migrations.qrc

# Zone
OTHER_FILES += conf/zone/*.json

RESOURCES += conf/conf-zone.qrc

# Shadow Build: Copy i18n/ to build path
!equals(PWD, $${OUT_PWD}) {
    i18n.files = $$files(i18n/*.qm)
    i18n.path = $${OUT_PWD}/i18n
    COPIES += i18n
}

# Windows
RC_FILE = FortFirewall.rc
OTHER_FILES += $${RC_FILE}

# Visual Leak Detector
visual_leak_detector {
    VLD_PATH = D:/Utils/Dev/VisualLeakDetector

    # 1) On the Project tab, under Build & Run / Build Steps / Details,
    # append to the Additional Arguments: CONFIG+=visual_leak_detector
    # 2) On the Project tab, under Build & Run / Run / Run Environment,
    # append to the PATH: $$VLD_PATH/bin/Win32

    INCLUDEPATH += $$VLD_PATH/include/
    LIBS += -L"$$VLD_PATH/lib/Win64"
    DEFINES += USE_VISUAL_LEAK_DETECTOR VLD_FORCE_ENABLE
}

# 3rd party integrations
CONFIG += qcustomplot sqlite
include(3rdparty/3rdparty.pri)

# Optional defines
DEFINES += USE_CONTROL_COMMANDS APP_SINGLE_INSTANCE
