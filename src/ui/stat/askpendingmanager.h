#ifndef ASKPENDINGMANAGER_H
#define ASKPENDINGMANAGER_H

#include <QObject>

#include <sqlite/sqlitetypes.h>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>

class LogEntryBlockedIp;

class AskPendingManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit AskPendingManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(AskPendingManager)

    SqliteDb *sqliteDb() const { return m_sqliteDb.data(); }

    void setUp() override;

    void logBlockedIp(const LogEntryBlockedIp &entry);

private:
    bool setupDb();

private:
    SqliteDbPtr m_sqliteDb;
};

#endif // ASKPENDINGMANAGER_H
