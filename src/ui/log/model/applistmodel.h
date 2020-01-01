#ifndef APPLISTMODEL_H
#define APPLISTMODEL_H

#include <QHash>
#include <QSet>

#include "../../util/model/stringlistmodel.h"

QT_FORWARD_DECLARE_CLASS(ConfManager)
QT_FORWARD_DECLARE_CLASS(IpListModel)
QT_FORWARD_DECLARE_CLASS(LogEntryBlocked)

class AppListModel : public StringListModel
{
    Q_OBJECT

public:
    explicit AppListModel(ConfManager *confManager,
                          QObject *parent = nullptr);

    void addLogEntry(const LogEntryBlocked &logEntry);

    IpListModel *ipListModel(const QString &appPath) const;

public slots:
    void clear() override;

    void remove(int row = -1) override;

private:
    QHash<QString, QStringList> m_appIpList;
    QHash<QString, QSet<QString>> m_appIpSet;

    ConfManager *m_confManager = nullptr;
    IpListModel *m_ipListModel = nullptr;
};

#endif // APPLISTMODEL_H
