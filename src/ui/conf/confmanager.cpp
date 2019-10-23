#include "confmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../fortcommon.h"
#include "../util/dateutil.h"
#include "../util/fileutil.h"
#include "../util/osutil.h"
#include "firewallconf.h"

Q_DECLARE_LOGGING_CATEGORY(CLOG_CONF_MANAGER)
Q_LOGGING_CATEGORY(CLOG_CONF_MANAGER, "fort.confManager")

#define logWarning() qCWarning(CLOG_CONF_MANAGER,)
#define logCritical() qCCritical(CLOG_CONF_MANAGER,)

#define DATABASE_USER_VERSION   1

namespace {

const char * const sqlPragmas =
        "PRAGMA locking_mode = EXCLUSIVE;"
        "PRAGMA synchronous = NORMAL;"
        ;

}

ConfManager::ConfManager(const QString &filePath,
                         QObject *parent) :
    QObject(parent),
    m_sqliteDb(new SqliteDb(filePath))
{
}

bool ConfManager::initialize()
{
    if (!m_sqliteDb->open()) {
        logCritical() << "File open error:"
                      << m_sqliteDb->filePath()
                      << m_sqliteDb->errorMessage();
        return false;
    }

    m_sqliteDb->execute(sqlPragmas);

    if (!m_sqliteDb->migrate(":/conf/migrations", DATABASE_USER_VERSION)) {
        logCritical() << "Migration error"
                      << m_sqliteDb->filePath();
        return false;
    }

    return true;
}
