#include "askpendingmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/ioc/ioccontainer.h>

#include "logblockedipjob.h"
#include "statsql.h"

namespace {

const QLoggingCategory LC("pendingManager");

}

AskPendingManager::AskPendingManager(QObject *parent) :
    QObject(parent), m_sqliteDb(new SqliteDb(":memory:"))
{
}

void AskPendingManager::setUp()
{
    setupDb();
}

bool AskPendingManager::setupDb()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return false;
    }

    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/stat/migrations/block",
        .recreate = true,
    };

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return false;
    }

    return true;
}

void AskPendingManager::logBlockedIp(const LogEntryBlockedIp &entry)
{
    // TODO
    Q_UNUSED(entry);
}
