#include "applistmodel.h"

#include <QIcon>

#include "../../conf/confmanager.h"
#include "../../util/app/appinfocache.h"
#include "../../util/fileutil.h"
#include "../../util/net/netutil.h"
#include "../logentryblocked.h"

#define IP_LIST_SIZE_MAX    64

AppListModel::AppListModel(ConfManager *confManager,
                           QObject *parent) :
    TableItemModel(parent),
    m_confManager(confManager)
{
    connect(m_confManager, &ConfManager::appEndTimesUpdated, this, &AppListModel::refresh);
}

void AppListModel::setAppInfoCache(AppInfoCache *v)
{
    m_appInfoCache = v;

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &AppListModel::refresh);
}

void AppListModel::initialize()
{
    if (appGroupNames().isEmpty()) {
        updateAppGroupNames();
    }

    connect(m_confManager, &ConfManager::confSaved, this, [&] {
        updateAppGroupNames();
        reset();
    });
}

void AppListModel::addLogEntry(const LogEntryBlocked &logEntry)
{
    const QString appPath = logEntry.path();

#if 0
    const QString ipText = NetUtil::ip4ToText(logEntry.ip())
            + ", " + NetUtil::protocolName(logEntry.proto())
            + ':' + QString::number(logEntry.port());
#endif

    if (confManager()->addApp(appPath, QDateTime(), 0, true, true, true)) {
        reset();
    }
}

int AppListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    if (m_appCount < 0) {
        m_appCount = confManager()->appCount();
    }

    return m_appCount;
}

int AppListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 5;
}

QVariant AppListModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const
{
    if (orientation == Qt::Horizontal
            && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return tr("Program");
        case 1: return tr("Group");
        case 2: return tr("Gr.");
        case 3: return tr("State");
        case 4: return tr("End Time");
        }
    }
    return QVariant();
}

QVariant AppListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole: {
        const int row = index.row();
        const int column = index.column();

        updateRowCache(row);

        switch (column) {
        case 0: {
            const auto appInfo = appInfoCache()->appInfo(m_rowCache.appPath);
            if (!appInfo.fileDescription.isEmpty()) {
                return appInfo.fileDescription;
            }

            return FileUtil::fileName(m_rowCache.appPath);
        }
        case 1: return appGroupNameByIndex(m_rowCache.groupIndex);
        case 3: return appStateToString(m_rowCache.state);
        case 4: return m_rowCache.endTime.isValid()
                    ? m_rowCache.endTime : QVariant();
        }

        break;
    }

    // Icon
    case Qt::DecorationRole: {
        const int row = index.row();
        const int column = index.column();

        updateRowCache(row);

        switch (column) {
        case 0: {
            const auto appPath = m_rowCache.appPath;
            const auto appInfo = appInfoCache()->appInfo(appPath);
            const auto appIcon = appInfoCache()->appIcon(appInfo);
            if (!appIcon.isNull()) {
                return QIcon(QPixmap::fromImage(appIcon));
            }

            return QIcon(":/images/application-window-96.png");
        }
        case 3: {
            switch (m_rowCache.state) {
            case AppAlert:
                return QIcon(":/images/error.png");
            case AppBlock:
                return QIcon(":/images/stop.png");
            case AppAllow:
                return QIcon(":/images/arrow_switch.png");
            }
        }
        }

        break;
    }

    // Text Alignment
    case Qt::TextAlignmentRole: {
        const int column = index.column();

        if (column >= 1 && column <= 3) {
            return int(Qt::AlignHCenter | Qt::AlignVCenter);
        }

        break;
    }

    // Check State
    case Qt::CheckStateRole: {
        if (index.column() == 2) {
            updateRowCache(index.row());
            return m_rowCache.useGroupPerm;
        }

        break;
    }
    }

    return QVariant();
}

QString AppListModel::appPathByRow(int row) const
{
    updateRowCache(row);

    return m_rowCache.appPath;
}

AppRow AppListModel::appRow(int row) const
{
    updateRowCache(row);

    return m_rowCache;
}

bool AppListModel::addApp(const QString &appPath, int groupIndex,
                          bool useGroupPerm, bool blocked,
                          const QDateTime &endTime)
{
    if (confManager()->updateDriverUpdateApp(appPath, groupIndex,
                                             useGroupPerm, blocked, false)
            && confManager()->addApp(appPath, endTime, groupIndex,
                                     useGroupPerm, blocked, false)) {
        reset();
        return true;
    }
    return false;
}

bool AppListModel::updateApp(qint64 appId, const QString &appPath,
                             int groupIndex, bool useGroupPerm, bool blocked,
                             const QDateTime &endTime)
{
    if (confManager()->updateDriverUpdateApp(appPath, groupIndex,
                                             useGroupPerm, blocked, false)
            && confManager()->updateApp(appId, endTime, groupIndex,
                                        useGroupPerm, blocked)) {
        refresh();
        return true;
    }
    return false;
}

void AppListModel::deleteApp(qint64 appId, const QString &appPath)
{
    if (confManager()->updateDriverDeleteApp(appPath)
            && confManager()->deleteApp(appId)) {
        reset();
    }
}

void AppListModel::reset()
{
    invalidateRowCache();
    TableItemModel::reset();
}

void AppListModel::refresh()
{
    invalidateRowCache();
    TableItemModel::refresh();
}

void AppListModel::invalidateRowCache()
{
    m_appCount = -1;
    m_rowCache.invalidate();
}

void AppListModel::updateRowCache(int row) const
{
    if (m_rowCache.isValid(row))
        return;

    bool blocked = false;
    bool alerted = false;

    if (m_confManager->getAppByIndex(m_rowCache.useGroupPerm, blocked, alerted,
                                     m_rowCache.appId, m_rowCache.groupIndex,
                                     m_rowCache.appPath, m_rowCache.endTime, row)) {
        m_rowCache.state = alerted ? AppAlert : (blocked ? AppBlock : AppAllow);
        m_rowCache.row = row;
    }
}

void AppListModel::updateAppGroupNames()
{
    m_appGroupNames = confManager()->appGroupNames();
}

QString AppListModel::appGroupNameByIndex(int groupIndex) const
{
    if (groupIndex < 0 || groupIndex >= m_appGroupNames.size())
        return QString();

    return m_appGroupNames.at(groupIndex);
}

QString AppListModel::appStateToString(AppState state) const
{
    switch (state) {
    case AppAlert: return tr("Alert");
    case AppBlock: return tr("Block");
    case AppAllow: return tr("Allow");
    }

    Q_UNREACHABLE();
    return QString();
}
