#include "appstatmodel.h"

#include "../../db/databasemanager.h"
#include "traflistmodel.h"

AppStatModel::AppStatModel(DatabaseManager *databaseManager,
                           QObject *parent) :
    StringListModel(parent),
    m_databaseManager(databaseManager),
    m_trafListModel(new TrafListModel(databaseManager, this))
{
}

void AppStatModel::initialize()
{
    updateList();
}

TrafListModel *AppStatModel::trafListModel(int trafType, int row,
                                           const QString &appPath) const
{
    Q_UNUSED(appPath)  // used to properly refresh trafListModel

    Q_ASSERT(row < m_appIds.size());

    m_trafListModel->setType(static_cast<TrafListModel::TrafType>(trafType));
    m_trafListModel->setAppId(row < 0 ? 0 : m_appIds.at(row));
    m_trafListModel->reset();

    return m_trafListModel;
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

    m_databaseManager->deleteApp(appId);

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

    m_databaseManager->getAppList(list, m_appIds);

    setList(list);
}

void AppStatModel::handleProcNew(const QString &appPath)
{
    bool isNew = false;
    const qint64 appId = m_databaseManager->logProcNew(appPath, isNew);

    if (isNew) {
        m_appIds.append(appId);
        insert(appPath);
    }
}

void AppStatModel::handleStatTraf(quint16 procCount, const quint8 *procBits,
                                  const quint32 *trafBytes)
{
    m_databaseManager->logStatTraf(procCount, procBits, trafBytes);
}
