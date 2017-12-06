#ifndef APPBLOCKEDMODEL_H
#define APPBLOCKEDMODEL_H

#include <QHash>
#include <QSet>

#include "stringlistmodel.h"

QT_FORWARD_DECLARE_CLASS(IpListModel)
QT_FORWARD_DECLARE_CLASS(LogEntryBlocked)

class AppBlockedModel : public StringListModel
{
    Q_OBJECT

public:
    explicit AppBlockedModel(QObject *parent = nullptr);

    void addLogEntry(const LogEntryBlocked &logEntry);

signals:

public slots:
    IpListModel *ipListModel(const QString &appPath) const;

    void clear() override;

private:
    QHash<QString, QStringList> m_appIpList;
    QHash<QString, QSet<QString>> m_appIpSet;

    IpListModel *m_ipListModel;
};

#endif // APPBLOCKEDMODEL_H
