#include "appstatmodel.h"

#include "../../db/databasemanager.h"

AppStatModel::AppStatModel(DatabaseManager *databaseManager,
                           QObject *parent) :
    StringListModel(parent),
    m_databaseManager(databaseManager)
{
}

void AppStatModel::initialize()
{
    m_databaseManager->initialize();

    updateList();
}

void AppStatModel::clear()
{
    StringListModel::clear();
}

void AppStatModel::updateList()
{
    setList(m_databaseManager->getAppList());
}

void AppStatModel::handleProcNew(const QString &appPath)
{
    bool isNew = false;
    m_databaseManager->addApp(appPath, isNew);

    if (isNew) {
        insert(appPath);
    }
}

void AppStatModel::handleStatTraf(quint16 procCount, const quint8 *procBits,
                                  const quint32 *trafBytes)
{
    m_databaseManager->addTraffic(procCount, procBits, trafBytes);
}
