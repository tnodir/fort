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

TrafListModel *AppStatModel::trafListModel(int trafType, int row) const
{
    m_trafListModel->setType(static_cast<TrafListModel::TrafType>(trafType));
    m_trafListModel->setAppId(row == -1 ? 0 : m_appIds.at(row));
    m_trafListModel->reset();

    return m_trafListModel;
}

void AppStatModel::clear()
{
    StringListModel::clear();
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
    m_databaseManager->logProcNew(appPath, isNew);

    if (isNew) {
        insert(appPath);
    }
}

void AppStatModel::handleStatTraf(quint16 procCount, const quint8 *procBits,
                                  const quint32 *trafBytes)
{
    m_databaseManager->logStatTraf(procCount, procBits, trafBytes);
}
