#include "appstatmodel.h"

#include <QIcon>

#include "../log/logentryprocnew.h"
#include "../log/logentrystattraf.h"
#include "../stat/statmanager.h"
#include "../util/app/appinfo.h"
#include "../util/app/appinfocache.h"
#include "../util/fileutil.h"
#include "../util/iconcache.h"
#include "traflistmodel.h"

AppStatModel::AppStatModel(StatManager *statManager, QObject *parent) :
    StringListModel(parent),
    m_statManager(statManager),
    m_trafListModel(new TrafListModel(statManager, this))
{
    connect(m_statManager, &StatManager::appCreated, this, &AppStatModel::handleCreatedApp);
}

void AppStatModel::setAppInfoCache(AppInfoCache *v)
{
    m_appInfoCache = v;

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &AppStatModel::refresh);
}

void AppStatModel::initialize()
{
    Q_ASSERT(appInfoCache());

    updateList();
}

void AppStatModel::clear()
{
    m_trafListModel->clear();

    updateList();
}

void AppStatModel::remove(int row)
{
    row = adjustRow(row);

    if (Q_UNLIKELY(row < 0 || row >= m_appIds.size()))
        return;

    beginRemoveRows(QModelIndex(), row, row);

    const qint64 appId = m_appIds.at(row);
    const QString appPath = list().at(row);

    m_statManager->deleteApp(appId, appPath);

    m_appIds.remove(row);

    removeRow(row);

    endRemoveRows();
}

void AppStatModel::updateList()
{
    QStringList list;
    list.append(QString()); // All

    m_appIds.clear();
    m_appIds.append(0); // All

    m_statManager->getAppList(list, m_appIds);

    setList(list);
}

void AppStatModel::handleCreatedApp(qint64 appId, const QString &appPath)
{
    m_appIds.append(appId);
    insert(appPath);
}

void AppStatModel::handleProcNew(const LogEntryProcNew &procNewEntry)
{
    m_statManager->logProcNew(procNewEntry.pid(), procNewEntry.path());
}

void AppStatModel::handleStatTraf(const LogEntryStatTraf &statTrafEntry, qint64 unixTime)
{
    m_statManager->logStatTraf(statTrafEntry.procCount(), unixTime, statTrafEntry.procTrafBytes());
}

qint64 AppStatModel::appIdByRow(int row) const
{
    return (row < 0 || row >= m_appIds.size()) ? 0 : m_appIds.at(row);
}

QString AppStatModel::appPathByRow(int row) const
{
    return (row <= 0 || row >= list().size()) ? QString() : list().at(row);
}

Qt::ItemFlags AppStatModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
}

QVariant AppStatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        QVariant();

    // Label
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        const int row = index.row();
        if (row == 0)
            return tr("All");

        const auto appPath = list().at(row);
        const auto appInfo = appInfoCache()->appInfo(appPath);
        if (!appInfo.fileDescription.isEmpty()) {
            return appInfo.fileDescription;
        }

        return FileUtil::fileName(appPath);
    }

    // Icon
    if (role == Qt::DecorationRole) {
        const int row = index.row();
        if (row == 0) {
            return IconCache::icon(":/images/computer-96.png");
        }

        const auto appPath = list().at(row);
        return appInfoCache()->appIcon(appPath, ":/images/application-window-96.png");
    }

    return StringListModel::data(index, role);
}
