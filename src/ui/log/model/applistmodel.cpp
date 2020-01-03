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
}

void AppListModel::setAppInfoCache(AppInfoCache *v)
{
    m_appInfoCache = v;

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &AppListModel::refresh);
}

void AppListModel::addLogEntry(const LogEntryBlocked &logEntry)
{
    const QString appPath = logEntry.path();

#if 0
    const QString ipText = NetUtil::ip4ToText(logEntry.ip())
            + ", " + NetUtil::protocolName(logEntry.proto())
            + ':' + QString::number(logEntry.port());
#endif

    if (confManager()->addApp(appPath, QDateTime(), 0, true, true)) {
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
        case 1: return tr("Path");
        case 2: return tr("Group");
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

    // Label
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
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
        case 1: return m_rowCache.appPath;
        case 2: return m_rowCache.appGroupName;
        case 3: return appStateToString(m_rowCache.state);
        case 4: return m_rowCache.endTime.isValid()
                    ? m_rowCache.endTime : QVariant();
        }
    }

    // Icon
    if (role == Qt::DecorationRole) {
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
            case Alert:
                return QIcon(":/images/error.png");
            case Block:
                return QIcon(":/images/cancel.png");
            case Allow:
                return QIcon(":/images/accept.png");
            }
        }
        }
    }

    // Text Alignment
    if (role == Qt::TextAlignmentRole) {
        const int column = index.column();

        switch (column) {
        case 2:
        case 3: return int(Qt::AlignHCenter | Qt::AlignVCenter);
        }
    }

    return QVariant();
}

QString AppListModel::appPathByRow(int row) const
{
    updateRowCache(row);

    return m_rowCache.appPath;
}

void AppListModel::reset()
{
    beginResetModel();
    invalidateRowCache();
    endResetModel();
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

    if (m_confManager->getAppByIndex(blocked, alerted,
                                     m_rowCache.appId, m_rowCache.appGroupId,
                                     m_rowCache.appGroupName, m_rowCache.appPath,
                                     m_rowCache.endTime, row)) {
        m_rowCache.state = alerted ? Alert : (blocked ? Block : Allow);
        m_rowCache.row = row;
    }
}

QString AppListModel::appStateToString(AppState state) const
{
    switch (state) {
    case Alert: return tr("Alert");
    case Block: return tr("Block");
    case Allow: return tr("Allow");
    }

    Q_UNREACHABLE();
    return QString();
}
