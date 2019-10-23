#ifndef CONFMANAGER_H
#define CONFMANAGER_H

#include <QObject>

#include "../util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(SqliteDb)
QT_FORWARD_DECLARE_CLASS(SqliteStmt)

class ConfManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfManager(const QString &filePath,
                         QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfManager)

    bool initialize();

signals:

private:
    SqliteDb *m_sqliteDb;
};

#endif // CONFMANAGER_H
