#include "connlistmodel.h"

#include "../stat/statmanager.h"
#include "../log/logentryblockedip.h"

ConnListModel::ConnListModel(StatManager *statManager, QObject *parent) :
    TableItemModel(parent), m_statManager(statManager)
{
}

void ConnListModel::handleLogBlockedIp(const LogEntryBlockedIp &entry, qint64 unixTime)
{
    // const QString ipText = NetUtil::ip4ToText(logEntry.ip()) + ", "
    //        + NetUtil::protocolName(logEntry.proto()) + ':' +
    //        QString::number(logEntry.port());

    if (m_statManager->logBlockedIp(entry.inbound(), entry.blockReason(), entry.ipProto(),
                entry.localPort(), entry.remotePort(), entry.localIp(), entry.remoteIp(),
                entry.pid(), entry.path(), unixTime)) {
        reset();
    }
}

int ConnListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_connCount;
}

int ConnListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
}

QVariant ConnListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("Date");
        case 1:
            return tr("Download");
        case 2:
            return tr("Upload");
        case 3:
            return tr("Sum");
        }
    }
    return QVariant();
}

QVariant ConnListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
//        const int row = index.row();
//        const int column = index.column();

//        updateRowCache(row);

//        switch (column) {
//        case 0:
//            return formatTrafTime(m_rowCache.trafTime);
//        case 1:
//            return formatTrafUnit(m_rowCache.inBytes);
//        case 2:
//            return formatTrafUnit(m_rowCache.outBytes);
//        case 3:
//            return formatTrafUnit(m_rowCache.inBytes + m_rowCache.outBytes);
//        }
    }

    return QVariant();
}
