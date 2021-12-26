include(../global.pri)

include(FortFirewallUI-include.pri)

CONFIG -= debug_and_release
CONFIG += staticlib

TARGET = FortFirewallUILib
TEMPLATE = lib

SOURCES += \
    appinfo/appinfo.cpp \
    appinfo/appinfocache.cpp \
    appinfo/appinfojob.cpp \
    appinfo/appinfomanager.cpp \
    appinfo/appinfoutil.cpp \
    appinfo/appinfoworker.cpp \
    conf/addressgroup.cpp \
    conf/appgroup.cpp \
    conf/confmanager.cpp \
    conf/firewallconf.cpp \
    conf/inioptions.cpp \
    control/control.cpp \
    control/controlmanager.cpp \
    control/controlworker.cpp \
    driver/drivercommon.cpp \
    driver/drivermanager.cpp \
    driver/driverworker.cpp \
    form/controls/appinforow.cpp \
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
    form/dialog/dialogutil.cpp \
    form/dialog/passworddialog.cpp \
    form/graph/axistickerspeed.cpp \
    form/graph/graphplot.cpp \
    form/graph/graphwindow.cpp \
    form/opt/optionscontroller.cpp \
    form/opt/optionswindow.cpp \
    form/opt/pages/addresses/addressescolumn.cpp \
    form/opt/pages/addressespage.cpp \
    form/opt/pages/applicationspage.cpp \
    form/opt/pages/apps/appscolumn.cpp \
    form/opt/pages/optbasepage.cpp \
    form/opt/pages/optionspage.cpp \
    form/opt/pages/optmainpage.cpp \
    form/opt/pages/rulespage.cpp \
    form/opt/pages/schedulepage.cpp \
    form/opt/pages/servicespage.cpp \
    form/opt/pages/statisticspage.cpp \
    form/prog/programeditdialog.cpp \
    form/prog/programscontroller.cpp \
    form/prog/programswindow.cpp \
    form/stat/pages/connectionspage.cpp \
    form/stat/pages/statbasepage.cpp \
    form/stat/pages/statmainpage.cpp \
    form/stat/pages/trafficpage.cpp \
    form/stat/statisticscontroller.cpp \
    form/stat/statisticswindow.cpp \
    form/tray/traycontroller.cpp \
    form/tray/trayicon.cpp \
    form/zone/zonescontroller.cpp \
    form/zone/zoneswindow.cpp \
    fortmanager.cpp \
    fortsettings.cpp \
    hostinfo/hostinfo.cpp \
    hostinfo/hostinfocache.cpp \
    hostinfo/hostinfojob.cpp \
    hostinfo/hostinfomanager.cpp \
    log/logbuffer.cpp \
    log/logentry.cpp \
    log/logentryblocked.cpp \
    log/logentryblockedip.cpp \
    log/logentryprocnew.cpp \
    log/logentrystattraf.cpp \
    log/logentrytime.cpp \
    log/logmanager.cpp \
    manager/envmanager.cpp \
    manager/hotkeymanager.cpp \
    manager/logger.cpp \
    manager/nativeeventfilter.cpp \
    manager/translationmanager.cpp \
    manager/windowmanager.cpp \
    model/applistmodel.cpp \
    model/appstatmodel.cpp \
    model/connlistmodel.cpp \
    model/servicelistmodel.cpp \
    model/traflistmodel.cpp \
    model/zonelistmodel.cpp \
    model/zonesourcewrapper.cpp \
    model/zonetypewrapper.cpp \
    rpc/appinfomanagerrpc.cpp \
    rpc/confmanagerrpc.cpp \
    rpc/drivermanagerrpc.cpp \
    rpc/logmanagerrpc.cpp \
    rpc/quotamanagerrpc.cpp \
    rpc/rpcmanager.cpp \
    rpc/statmanagerrpc.cpp \
    rpc/taskmanagerrpc.cpp \
    rpc/windowmanagerfake.cpp \
    serviceinfo/serviceinfo.cpp \
    serviceinfo/serviceinfomanager.cpp \
    serviceinfo/serviceinfomonitor.cpp \
    stat/quotamanager.cpp \
    stat/statmanager.cpp \
    stat/statsql.cpp \
    task/taskdownloader.cpp \
    task/taskeditinfo.cpp \
    task/taskinfo.cpp \
    task/taskinfoupdatechecker.cpp \
    task/taskinfozonedownloader.cpp \
    task/tasklistmodel.cpp \
    task/taskmanager.cpp \
    task/taskupdatechecker.cpp \
    task/taskworker.cpp \
    task/taskzonedownloader.cpp \
    user/iniuser.cpp \
    user/usersettings.cpp \
    util/conf/addressrange.cpp \
    util/conf/confutil.cpp \
    util/dateutil.cpp \
    util/device.cpp \
    util/fileutil.cpp \
    util/guiutil.cpp \
    util/iconcache.cpp \
    util/ini/mapsettings.cpp \
    util/ini/settings.cpp \
    util/ioc/ioccontainer.cpp \
    util/json/jsonutil.cpp \
    util/json/mapwrapper.cpp \
    util/model/stringlistmodel.cpp \
    util/model/tableitemmodel.cpp \
    util/model/tablesqlmodel.cpp \
    util/net/ip4range.cpp \
    util/net/netdownloader.cpp \
    util/net/netutil.cpp \
    util/osutil.cpp \
    util/processinfo.cpp \
    util/regkey.cpp \
    util/serviceworker.cpp \
    util/startuputil.cpp \
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
    appinfo/appinfo.h \
    appinfo/appinfocache.h \
    appinfo/appinfojob.h \
    appinfo/appinfomanager.h \
    appinfo/appinfoutil.h \
    appinfo/appinfoworker.h \
    conf/addressgroup.h \
    conf/appgroup.h \
    conf/confmanager.h \
    conf/firewallconf.h \
    conf/inioptions.h \
    control/control.h \
    control/controlmanager.h \
    control/controlworker.h \
    driver/drivercommon.h \
    driver/drivermanager.h \
    driver/driverworker.h \
    form/controls/appinforow.h \
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
    form/dialog/dialogutil.h \
    form/dialog/passworddialog.h \
    form/graph/axistickerspeed.h \
    form/graph/graphplot.h \
    form/graph/graphwindow.h \
    form/opt/optionscontroller.h \
    form/opt/optionswindow.h \
    form/opt/pages/addresses/addressescolumn.h \
    form/opt/pages/addressespage.h \
    form/opt/pages/applicationspage.h \
    form/opt/pages/apps/appscolumn.h \
    form/opt/pages/optbasepage.h \
    form/opt/pages/optionspage.h \
    form/opt/pages/optmainpage.h \
    form/opt/pages/rulespage.h \
    form/opt/pages/schedulepage.h \
    form/opt/pages/servicespage.h \
    form/opt/pages/statisticspage.h \
    form/prog/programeditdialog.h \
    form/prog/programscontroller.h \
    form/prog/programswindow.h \
    form/stat/pages/connectionspage.h \
    form/stat/pages/statbasepage.h \
    form/stat/pages/statmainpage.h \
    form/stat/pages/trafficpage.h \
    form/stat/statisticscontroller.h \
    form/stat/statisticswindow.h \
    form/tray/traycontroller.h \
    form/tray/trayicon.h \
    form/zone/zonescontroller.h \
    form/zone/zoneswindow.h \
    fortcompat.h \
    fortmanager.h \
    fortsettings.h \
    hostinfo/hostinfo.h \
    hostinfo/hostinfocache.h \
    hostinfo/hostinfojob.h \
    hostinfo/hostinfomanager.h \
    log/logbuffer.h \
    log/logentry.h \
    log/logentryblocked.h \
    log/logentryblockedip.h \
    log/logentryprocnew.h \
    log/logentrystattraf.h \
    log/logentrytime.h \
    log/logmanager.h \
    manager/envmanager.h \
    manager/hotkeymanager.h \
    manager/logger.h \
    manager/nativeeventfilter.h \
    manager/translationmanager.h \
    manager/windowmanager.h \
    model/applistmodel.h \
    model/appstatmodel.h \
    model/connlistmodel.h \
    model/servicelistmodel.h \
    model/traflistmodel.h \
    model/zonelistmodel.h \
    model/zonesourcewrapper.h \
    model/zonetypewrapper.h \
    rpc/appinfomanagerrpc.h \
    rpc/confmanagerrpc.h \
    rpc/drivermanagerrpc.h \
    rpc/logmanagerrpc.h \
    rpc/quotamanagerrpc.h \
    rpc/rpcmanager.h \
    rpc/statmanagerrpc.h \
    rpc/taskmanagerrpc.h \
    rpc/windowmanagerfake.h \
    serviceinfo/serviceinfo.h \
    serviceinfo/serviceinfomanager.h \
    serviceinfo/serviceinfomonitor.h \
    stat/quotamanager.h \
    stat/statmanager.h \
    stat/statsql.h \
    task/taskdownloader.h \
    task/taskeditinfo.h \
    task/taskinfo.h \
    task/taskinfoupdatechecker.h \
    task/taskinfozonedownloader.h \
    task/tasklistmodel.h \
    task/taskmanager.h \
    task/taskupdatechecker.h \
    task/taskworker.h \
    task/taskzonedownloader.h \
    user/iniuser.h \
    user/usersettings.h \
    util/classhelpers.h \
    util/conf/addressrange.h \
    util/conf/confappswalker.h \
    util/conf/confutil.h \
    util/dateutil.h \
    util/device.h \
    util/fileutil.h \
    util/guiutil.h \
    util/iconcache.h \
    util/ini/mapsettings.h \
    util/ini/settings.h \
    util/ioc/ioccontainer.h \
    util/ioc/iocservice.h \
    util/json/jsonutil.h \
    util/json/mapwrapper.h \
    util/model/stringlistmodel.h \
    util/model/tableitemmodel.h \
    util/model/tablesqlmodel.h \
    util/net/ip4range.h \
    util/net/netdownloader.h \
    util/net/netutil.h \
    util/osutil.h \
    util/processinfo.h \
    util/regkey.h \
    util/serviceworker.h \
    util/startuputil.h \
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

# Icons, README.*
RESOURCES += \
    fort_icons.qrc \
    fort_readme.qrc

# Database Migrations
OTHER_FILES += \
    appinfo/migrations/*.sql \
    conf/migrations/*.sql \
    stat/migrations/*.sql

RESOURCES += \
    appinfo/appinfo_migrations.qrc \
    conf/conf_migrations.qrc \
    stat/stat_migrations.qrc

# Zone
OTHER_FILES += conf/zone/*.json

RESOURCES += \
    conf/conf_zone.qrc

# Driver integration
include(../driver/Driver.pri)

# 3rd party integrations
include(3rdparty/3rdparty.pri)
