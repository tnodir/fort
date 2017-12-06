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
    m_databaseManager->initialize();

    updateList();
}

TrafListModel *AppStatModel::trafListModel(int trafType,
                                           const QString &appPath) const
{
    if (appPath != m_trafListModel->appPath()) {
        m_trafListModel->setType(static_cast<TrafListModel::TrafType>(trafType));
        m_trafListModel->setAppPath(appPath);
        m_trafListModel->reset();
    }

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
    m_databaseManager->getAppList(list);

    setList(list);
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
