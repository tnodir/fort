#ifndef APPBLOCKEDMODEL_H
#define APPBLOCKEDMODEL_H

#include <QHash>
#include <QSet>

#include "stringlistmodel.h"

class IpListModel;
class LogEntryBlocked;

class AppBlockedModel : public StringListModel
{
    Q_OBJECT

public:
    explicit AppBlockedModel(QObject *parent = nullptr);

    void addLogEntry(const LogEntryBlocked &logEntry);

signals:

public slots:
    QAbstractItemModel *ipListModel(const QString &appPath) const;

    void clear();

private:
    static QString getEntryPath(const LogEntryBlocked &logEntry);

private:
    QHash<QString, QStringList> m_appIpList;
    QHash<QString, QSet<QString>> m_appIpSet;

    IpListModel *m_ipListModel;
};

#endif // APPBLOCKEDMODEL_H
