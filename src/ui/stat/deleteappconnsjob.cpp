#include "deleteappconnsjob.h"

#include <sqlite/dbquery.h>
#include <sqlite/dbutil.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitestmt.h>

#include "statsql.h"

DeleteAppConnsJob::DeleteAppConnsJob(qint64 confAppId, const QString &appPath) :
    DeleteConnIdsJob({}), m_confAppId(confAppId), m_appPath(appPath)
{
}

bool DeleteAppConnsJob::processMerge(const StatConnBaseJob &statJob)
{
    const auto &job = static_cast<const DeleteAppConnsJob &>(statJob);

    return confAppId() == job.confAppId() && appPath() == job.appPath();
}

void DeleteAppConnsJob::deleteConns()
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb())
                    .sql(StatSql::sqlDeleteAppConns)
                    .vars({ DbVar::nullable(confAppId()), DbVar::nullable(appPath()) })
                    .prepare(stmt))
        return;

    setConnIds(DbUtil::stepIds(&stmt));
}
