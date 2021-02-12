#ifndef CONNLISTMODEL_H
#define CONNLISTMODEL_H

#include "../util/model/stringlistmodel.h"

class ConfManager;
class LogEntryBlockedIp;

class ConnListModel : public StringListModel
{
    Q_OBJECT

public:
    explicit ConnListModel(ConfManager *confManager, QObject *parent = nullptr);

    QString appPath() const { return m_appPath; }
    void setAppPath(const QString &appPath);

    void handleLogBlockedIp(const LogEntryBlockedIp &logEntry);

public slots:
    void clear() override;

private:
    ConfManager *m_confManager = nullptr;

    QString m_appPath;
};

#endif // CONNLISTMODEL_H
