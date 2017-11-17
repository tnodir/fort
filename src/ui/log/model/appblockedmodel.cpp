#include "appblockedmodel.h"

#include "../../util/fileutil.h"
#include "../../util/net/netutil.h"
#include "../../util/osutil.h"
#include "../logentryblocked.h"

#define IP_LIST_COUNT_MAX   64

AppBlockedModel::AppBlockedModel(QObject *parent) :
    StringListModel(parent)
{
}

void AppBlockedModel::addLogEntry(const LogEntryBlocked &logEntry)
{
    const QString appPath = getEntryPath(logEntry);
    const QString ipText = NetUtil::ip4ToText(logEntry.ip());
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
        insert(appPath);
    } else if (ipList.size() > IP_LIST_COUNT_MAX) {
        const QString oldIpText = ipList.takeLast();
        ipSet.remove(oldIpText);
    }
}

QString AppBlockedModel::getEntryPath(const LogEntryBlocked &logEntry)
{
    const QString kernelPath = logEntry.kernelPath();

    return kernelPath.isEmpty()
            ? OsUtil::pidToPath(logEntry.pid())
            : FileUtil::kernelPathToPath(kernelPath);
}
