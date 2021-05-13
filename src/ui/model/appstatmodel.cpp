#include "appstatmodel.h"

#include <QIcon>

#include "../appinfo/appinfo.h"
#include "../appinfo/appinfocache.h"
#include "../log/logentryprocnew.h"
#include "../log/logentrystattraf.h"
#include "../stat/statmanager.h"
#include "../util/fileutil.h"
#include "../util/iconcache.h"
#include "traflistmodel.h"

AppStatModel::AppStatModel(StatManager *statManager, QObject *parent) :
    StringListModel(parent),
    m_statManager(statManager),
    m_trafListModel(new TrafListModel(statManager, this))
{
    connect(m_statManager, &StatManager::appStatRemoved, this, &AppStatModel::onStatAppRemoved);
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

    doBeginRemoveRows(row, row);

    const qint64 appId = m_appIds.at(row);

    m_statManager->deleteStatApp(appId);

    m_appIds.remove(row);
    removeRow(row);

    doEndRemoveRows();
}

void AppStatModel::onStatAppRemoved(qint64 appId)
{
    if (isChanging())
        return;

    const int row = m_appIds.indexOf(appId);
    if (row == -1)
        return;

    doBeginRemoveRows(row, row);

    m_appIds.remove(row);
    removeRow(row);

    doEndRemoveRows();
}

void AppStatModel::updateList()
{
    QStringList list;
    list.append(QString()); // All

    m_appIds.clear();
    m_appIds.append(0); // All

    m_statManager->getStatAppList(list, m_appIds);

    setList(list);
}

void AppStatModel::handleCreatedApp(qint64 appId, const QString &appPath)
{
    m_appIds.append(appId);
    insert(appPath);
}

void AppStatModel::handleLogProcNew(const LogEntryProcNew &entry, qint64 unixTime)
{
    m_statManager->logProcNew(entry.pid(), entry.path(), unixTime);
}

void AppStatModel::handleLogStatTraf(const LogEntryStatTraf &entry, qint64 unixTime)
{
    m_statManager->logStatTraf(entry.procCount(), entry.procTrafBytes(), unixTime);
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
    if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
        return dataDisplay(index);

    // Icon
    if (role == Qt::DecorationRole)
        return dataDecoration(index);

    return StringListModel::data(index, role);
}

QVariant AppStatModel::dataDisplay(const QModelIndex &index) const
{
    const int row = index.row();
    if (row == 0)
        return tr("All");

    const auto appPath = list().at(row);
    return appInfoCache()->appName(appPath);
}

QVariant AppStatModel::dataDecoration(const QModelIndex &index) const
{
    const int row = index.row();
    if (row == 0) {
        return IconCache::icon(":/icons/computer-96.png");
    }

    const auto appPath = list().at(row);
    return appInfoCache()->appIcon(appPath);
}
