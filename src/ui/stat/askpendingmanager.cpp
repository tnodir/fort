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
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return;
    }

    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/stat/migrations/block",
        .recreate = true,
    };

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return;
    }
}

void AskPendingManager::logBlockedIp(const LogEntryBlockedIp &entry) { }
