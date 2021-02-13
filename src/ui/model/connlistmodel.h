#ifndef CONNLISTMODEL_H
#define CONNLISTMODEL_H

#include "../util/model/tableitemmodel.h"

class LogEntryBlockedIp;
class StatManager;

struct ConnRow : CacheRow
{
    qint32 trafTime = 0;
};

class ConnListModel : public TableItemModel
{
    Q_OBJECT

public:
    explicit ConnListModel(StatManager *statManager, QObject *parent = nullptr);

    void handleLogBlockedIp(const LogEntryBlockedIp &entry, qint64 unixTime);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    StatManager *m_statManager = nullptr;

    int m_connCount = 0;

    mutable ConnRow m_rowCache;
};

#endif // CONNLISTMODEL_H
