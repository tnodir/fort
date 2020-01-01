#include "applistmodel.h"

#include "../../conf/confmanager.h"
#include "../../util/net/netutil.h"
#include "../logentryblocked.h"
#include "iplistmodel.h"

#define IP_LIST_SIZE_MAX    64

AppListModel::AppListModel(ConfManager *confManager,
                           QObject *parent) :
    StringListModel(parent),
    m_confManager(confManager),
    m_ipListModel(new IpListModel(this))
{
}

IpListModel *AppListModel::ipListModel(const QString &appPath) const
{
    if (appPath != m_ipListModel->appPath()) {
        m_ipListModel->setAppPath(appPath);
        m_ipListModel->setList(m_appIpList.value(appPath));
    }

    return m_ipListModel;
}

void AppListModel::clear()
{
    m_appIpList.clear();
    m_appIpSet.clear();

    m_ipListModel->clear();

    StringListModel::clear();
}

void AppListModel::remove(int row)
{
    row = adjustRow(row);

    beginRemoveRows(QModelIndex(), row, row);

    const QString appPath = list().at(row);

    m_appIpList.remove(appPath);
    m_appIpSet.remove(appPath);

    removeRow(row);

    endRemoveRows();
}

void AppListModel::addLogEntry(const LogEntryBlocked &logEntry)
{
    const QString appPath = logEntry.path();
    const QString ipText = NetUtil::ip4ToText(logEntry.ip())
            + ", " + NetUtil::protocolName(logEntry.proto())
            + ':' + QString::number(logEntry.port());
    bool isNewApp = false;

    if (!m_appIpList.contains(appPath)) {
        m_appIpList.insert(appPath, QStringList());
        m_appIpSet.insert(appPath, QSet<QString>());
        isNewApp = true;
    }

    QSet<QString> &ipSet = m_appIpSet[appPath];
    if (ipSet.contains(ipText))
        return;

    ipSet.insert(ipText);

    QStringList &ipList = m_appIpList[appPath];
    ipList.prepend(ipText);

    if (isNewApp) {
        insert(appPath);  // update at the end to refresh the view
    } else {
        if (ipList.size() > IP_LIST_SIZE_MAX) {
            const QString oldIpText = ipList.takeLast();
            ipSet.remove(oldIpText);
        }

        if (appPath == m_ipListModel->appPath()) {
            m_ipListModel->setList(ipList);
        }
    }
}
