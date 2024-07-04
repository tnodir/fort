#include "applistmodeldata.h"

#include <QFont>
#include <QIcon>

#include <appinfo/appinfocache.h>
#include <conf/app.h>
#include <conf/appgroup.h>
#include <conf/confappmanager.h>
#include <conf/firewallconf.h>
#include <util/bitutil.h>
#include <util/dateutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

#include "applistmodel.h"

namespace {

const auto alertColor = QColor("orange");
const auto allowColor = QColor("green");
const auto blockColor = QColor("red");
const auto killProcessColor = QColor("magenta");
const auto inactiveColor = QColor("slategray");

QVariant dataDisplayName(const App &app, int role)
{
    return app.appName
            + (role == Qt::ToolTipRole && !app.notes.isEmpty() ? "\n\n" + app.notes : QString());
}

QVariant dataDisplayAction(const App &app, int role)
{
    if (app.killProcess)
        return AppListModel::tr("Kill Process");

    if (app.blocked)
        return AppListModel::tr("Block");

    if (role == Qt::ToolTipRole && app.lanOnly)
        return AppListModel::tr("Block Internet Traffic");

    return AppListModel::tr("Allow");
}

QVariant dataDisplayZones(const App &app, int role)
{
    if (role != Qt::ToolTipRole)
        return QString();

    QString countText;
    if (app.acceptZones != 0) {
        const int acceptZonesCount = BitUtil::bitCount(app.acceptZones);
        countText += QString::number(acceptZonesCount);
    }
    if (app.rejectZones != 0) {
        const int rejectZonesCount = BitUtil::bitCount(app.rejectZones);
        countText += '^' + QString::number(rejectZonesCount);
    }

    return countText;
}

QVariant dataDisplayRule(const App &app, int role)
{
    if (role != Qt::ToolTipRole)
        return QString();

    return app.ruleName;
}

QVariant dataDisplayScheduled(const App &app, int role)
{
    if (role != Qt::ToolTipRole || app.scheduleTime.isNull())
        return QString();

    return DateUtil::localeDateTime(app.scheduleTime);
}

QVariant dataDisplayGroup(const App &app, int /*role*/)
{
    const FirewallConf *conf = IoC<ConfAppManager>()->conf();

    const AppGroup *appGroup = conf->appGroupAt(app.groupIndex);

    return appGroup->name();
}

QVariant dataDisplayFilePath(const App &app, int /*role*/)
{
    return app.appOriginPath;
}

QVariant dataDisplayCreationTime(const App &app, int /*role*/)
{
    return app.creatTime;
}

using dataDisplay_func = QVariant (*)(const App &app, int role);

static const dataDisplay_func dataDisplay_funcList[] = {
    &dataDisplayName,
    &dataDisplayZones,
    &dataDisplayRule,
    &dataDisplayScheduled,
    &dataDisplayAction,
    &dataDisplayGroup,
    &dataDisplayFilePath,
    &dataDisplayCreationTime,
};

QVariant headerDataDisplayName(int /*role*/)
{
    return AppListModel::tr("Name");
}

QVariant headerDataDisplayZones(int role)
{
    return (role == Qt::ToolTipRole) ? AppListModel::tr("Zones") : QString();
}

QVariant headerDataDisplayRule(int role)
{
    return (role == Qt::ToolTipRole) ? AppListModel::tr("Rule") : QString();
}

QVariant headerDataDisplayScheduled(int role)
{
    return (role == Qt::ToolTipRole) ? AppListModel::tr("Scheduled") : QString();
}

QVariant headerDataDisplayAction(int /*role*/)
{
    return AppListModel::tr("Action");
}

QVariant headerDataDisplayGroup(int /*role*/)
{
    return AppListModel::tr("Group");
}

QVariant headerDataDisplayFilePath(int /*role*/)
{
    return AppListModel::tr("File Path");
}

QVariant headerDataDisplayCreationTime(int /*role*/)
{
    return AppListModel::tr("Creation Time");
}

using headerDataDisplay_func = QVariant (*)(int role);

static const headerDataDisplay_func headerDataDisplay_funcList[] = {
    &headerDataDisplayName,
    &headerDataDisplayZones,
    &headerDataDisplayRule,
    &headerDataDisplayScheduled,
    &headerDataDisplayAction,
    &headerDataDisplayGroup,
    &headerDataDisplayFilePath,
    &headerDataDisplayCreationTime,
};

}

AppListModelData::AppListModelData(const App &app, const QModelIndex &index, int role) :
    m_role(role), m_index(index), m_app(app)
{
}

QString AppListModelData::appActionIconPath() const
{
    if (app().alerted)
        return ":/icons/error.png";

    if (app().killProcess)
        return ":/icons/scull.png";

    if (app().blocked)
        return ":/icons/deny.png";

    if (app().lanOnly)
        return ":/icons/hostname.png";

    return ":/icons/accept.png";
}

QString AppListModelData::appScheduleIconPath() const
{
    switch (app().scheduleAction) {
    case App::ScheduleBlock:
        return ":/icons/deny.png";
    case App::ScheduleAllow:
        return ":/icons/accept.png";
    case App::ScheduleRemove:
        return ":/icons/delete.png";
    case App::ScheduleKillProcess:
        return ":/icons/scull.png";
    }

    return {};
}

QColor AppListModelData::appActionColor() const
{
    if (app().killProcess)
        return killProcessColor;

    if (app().blocked)
        return blockColor;

    return allowColor;
}

QVariant AppListModelData::appGroupColor() const
{
    if (!app().useGroupPerm)
        return inactiveColor;

    const FirewallConf *conf = IoC<ConfAppManager>()->conf();

    const AppGroup *appGroup = conf->appGroupAt(app().groupIndex);
    if (!appGroup->enabled())
        return blockColor;

    return {};
}

QIcon AppListModelData::appIcon() const
{
    if (app().isWildcard) {
        return IconCache::icon(":/icons/coding.png");
    }

    return IoC<AppInfoCache>()->appIcon(app().appPath);
}

QIcon AppListModelData::appZonesIcon() const
{
    return app().hasZone() ? IconCache::icon(":/icons/ip_class.png") : QIcon();
}

QIcon AppListModelData::appRuleIcon() const
{
    return (app().ruleId != 0) ? IconCache::icon(":/icons/script.png") : QIcon();
}

QIcon AppListModelData::appScheduledIcon() const
{
    if (app().scheduleTime.isNull())
        return QIcon();

    const AppListModelData data(app());

    return IconCache::icon(data.appScheduleIconPath());
}

QIcon AppListModelData::appActionIcon() const
{
    return IconCache::icon(appActionIconPath());
}

QVariant AppListModelData::dataDecorationIcon() const
{
    switch (column()) {
    case 0:
        return appIcon();
    case 1:
        return appZonesIcon();
    case 2:
        return appRuleIcon();
    case 3:
        return appScheduledIcon();
    case 4:
        return appActionIcon();
    }

    return {};
}

QVariant AppListModelData::dataForeground() const
{
    switch (column()) {
    case 4:
        return appActionColor();
    case 5:
        return appGroupColor();
    }

    return {};
}

QVariant AppListModelData::dataDisplayRow() const
{
    const dataDisplay_func func = dataDisplay_funcList[column()];

    return func(app(), role());
}

QVariant AppListModelData::headerDataDisplay(int column, int role)
{
    const headerDataDisplay_func func = headerDataDisplay_funcList[column];

    return func(role);
}

QVariant AppListModelData::headerDataDecoration(int column)
{
    switch (column) {
    case 1:
        return IconCache::icon(":/icons/ip_class.png");
    case 2:
        return IconCache::icon(":/icons/script.png");
    case 3:
        return IconCache::icon(":/icons/time.png");
    }
    return {};
}
