#include "updateconnhostsjob.h"

#include <algorithm>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/net/netdbutil.h>

#include "statconnmanager.h"
#include "statsql.h"

UpdateConnHostsJob::UpdateConnHostsJob(const ConnHost &connHost)
{
    m_connHosts.append(connHost);
}

bool UpdateConnHostsJob::processMerge(const StatConnBaseJob &statJob)
{
    const auto &job = static_cast<const UpdateConnHostsJob &>(statJob);

    m_connHosts.append(job.connHosts());

    return true;
}

void UpdateConnHostsJob::processJob()
{
    int resultCount = 0;

    beginWriteTransaction();

    for (const ConnHost &connHost : connHosts()) {
        resultCount += updateConnHost(connHost);
    }

    commitTransaction();

    setResultCount(resultCount);
}

void UpdateConnHostsJob::emitFinished()
{
    if (resultCount() == 0)
        return;

    const auto it = std::min_element(m_connHosts.cbegin(), m_connHosts.cend(),
            [](const ConnHost &a, const ConnHost &b) { return a.connIdMin < b.connIdMin; });

    emit manager()->connHostsChanged(it->connIdMin);
}

int UpdateConnHostsJob::updateConnHost(const ConnHost &connHost)
{
    const ProcessIpKey &key = connHost.key;
    const bool isIPv6 = (key.isIPv6 != 0);

    SqliteStmt *stmt = getStmt(StatSql::sqlUpdateConnHost);

    stmt->bindVars({ connHost.name, connHost.connIdMin, key.pid, NetDbUtil::ip4Var(key.ip, isIPv6),
            NetDbUtil::ip6Var(key.ip, isIPv6) });

    if (!sqliteDb()->done(stmt))
        return 0;

    return sqliteDb()->changes();
}
