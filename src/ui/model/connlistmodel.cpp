#include "connlistmodel.h"

#include "../log/logentryblockedip.h"

ConnListModel::ConnListModel(ConfManager *confManager, QObject *parent) :
    StringListModel(parent), m_confManager(confManager)
{
}

void ConnListModel::setAppPath(const QString &appPath)
{
    m_appPath = appPath;
}

void ConnListModel::handleLogBlockedIp(const LogEntryBlockedIp &logEntry)
{
    const QString appPath = logEntry.path();

#if 0
    const QString ipText = NetUtil::ip4ToText(logEntry.ip())
            + ", " + NetUtil::protocolName(logEntry.proto())
            + ':' + QString::number(logEntry.port());

    if (confManager()->addApp(
                appPath, QString(), QDateTime(), groupId, false, logEntry.blocked(), true)) {
        reset();
    }
#endif
}

void ConnListModel::clear()
{
    m_appPath = QString();

    StringListModel::clear();
}
