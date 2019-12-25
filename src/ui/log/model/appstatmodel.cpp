#include "appstatmodel.h"

#include "../../stat/statmanager.h"
#include "../logentryprocnew.h"
#include "../logentrystattraf.h"
#include "traflistmodel.h"

AppStatModel::AppStatModel(StatManager *statManager,
                           QObject *parent) :
    StringListModel(parent),
    m_statManager(statManager),
    m_trafListModel(new TrafListModel(statManager, this))
{
    connect(m_statManager, &StatManager::appCreated,
            this, &AppStatModel::handleCreatedApp);
}

void AppStatModel::initialize()
{
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

    beginRemoveRows(QModelIndex(), row, row);

    const qint64 appId = m_appIds.at(row);

    m_statManager->deleteApp(appId);

    m_appIds.remove(row);

    removeRow(row);

    endRemoveRows();
}

void AppStatModel::updateList()
{
    QStringList list;
    list.append(QString());  // All

    m_appIds.clear();
    m_appIds.append(0);  // All

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
    m_statManager->logProcNew(procNewEntry.pid(),
                              procNewEntry.path());
}

void AppStatModel::handleStatTraf(const LogEntryStatTraf &statTrafEntry)
{
    m_statManager->logStatTraf(statTrafEntry.procCount(),
                               statTrafEntry.procTrafBytes());
}

qint64 AppStatModel::appIdByRow(int row) const
{
    return (row < 0 || row >= m_appIds.size())
            ? 0 : m_appIds.at(row);
}
