#include "appstatmodel.h"

#include <QIcon>

#include <appinfo/appinfo.h>
#include <appinfo/appinfocache.h>
#include <fortmanager.h>
#include <stat/statmanager.h>
#include <util/fileutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

#include "traflistmodel.h"

AppStatModel::AppStatModel(QObject *parent) : StringListModel(parent) { }

StatManager *AppStatModel::statManager() const
{
    return IoC<StatManager>();
}

AppInfoCache *AppStatModel::appInfoCache() const
{
    return IoC<AppInfoCache>();
}

void AppStatModel::initialize()
{
    updateList();

    connect(statManager(), &StatManager::appStatRemoved, this, &AppStatModel::onStatAppRemoved);
    connect(statManager(), &StatManager::appCreated, this, &AppStatModel::handleCreatedApp);

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &AppStatModel::refresh);
}

void AppStatModel::clear()
{
    updateList();
}

void AppStatModel::remove(int row)
{
    row = adjustRow(row);

    if (Q_UNLIKELY(row < 0 || row >= m_appIds.size()))
        return;

    doBeginRemoveRows(row, row);

    const qint64 appId = m_appIds.at(row);

    statManager()->deleteStatApp(appId);

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

    statManager()->getStatAppList(list, m_appIds);

    setList(list);
}

void AppStatModel::handleCreatedApp(qint64 appId, const QString &appPath)
{
    m_appIds.append(appId);
    insert(appPath);
}

qint64 AppStatModel::appIdByRow(int row) const
{
    return (row < 0 || row >= m_appIds.size()) ? 0 : m_appIds.at(row);
}

QString AppStatModel::appPathByRow(int row) const
{
    return (row <= 0 || row >= list().size()) ? QString() : list().at(row);
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

    const auto &appPath = list().at(row);
    return appInfoCache()->appName(appPath);
}

QVariant AppStatModel::dataDecoration(const QModelIndex &index) const
{
    const int row = index.row();
    if (row == 0) {
        return IconCache::icon(":/icons/computer-96.png");
    }

    const auto &appPath = list().at(row);
    return appInfoCache()->appIcon(appPath);
}
