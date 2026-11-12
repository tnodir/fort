#include "hostzonesdb.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/zone.h>
#include <util/conf/confutil.h>
#include <util/net/hostpattern.h>
#include <util/net/hostrange.h>

namespace {

const QLoggingCategory LC("dns.hostZonesDb");

inline constexpr int DATABASE_USER_VERSION = 1;

const char *const sqlSelectHost = "SELECT name_mask, sub_mask FROM host WHERE name = ?1;";

const char *const sqlUpsertHost = "INSERT INTO host(name, name_mask, sub_mask) VALUES(?1, ?2, ?3)"
                                  "  ON CONFLICT(name) DO UPDATE"
                                  "    SET name_mask = name_mask | ?2, sub_mask = sub_mask | ?3;";

const char *const sqlClearHosts =
        "UPDATE host SET name_mask = name_mask & ~?1, sub_mask = sub_mask & ~?1"
        "  WHERE ((name_mask | sub_mask) & ?1) <> 0;";

const char *const sqlSelectPatterns = "SELECT name, zones_mask FROM host_pattern;";

const char *const sqlUpsertPattern =
        "INSERT INTO host_pattern(name, zones_mask) VALUES(?1, ?2)"
        "  ON CONFLICT(name) DO UPDATE SET zones_mask = zones_mask | ?2;";

const char *const sqlClearPatterns = "UPDATE host_pattern SET zones_mask = zones_mask & ~?1"
                                     "  WHERE (zones_mask & ?1) <> 0;";

const char *const sqlDeleteEmptyNames = "DELETE FROM host WHERE name_mask = 0 AND sub_mask = 0;"
                                        "DELETE FROM host_pattern WHERE zones_mask = 0;";

const char *const sqlSelectZoneIds = "SELECT zone_id FROM host_zone;";

const char *const sqlSelectZone = "SELECT 1 FROM host_zone"
                                  "  WHERE zone_id = ?1 AND bin_checksum = ?2 AND subdomains = ?3;";

const char *const sqlUpsertZone =
        "INSERT OR REPLACE INTO host_zone(zone_id, bin_checksum, subdomains) VALUES(?1, ?2, ?3);";

const char *const sqlDeleteZone = "DELETE FROM host_zone WHERE zone_id = ?1;";

bool stepDone(SqliteStmt *stmt)
{
    const bool ok = (stmt->step() == SqliteStmt::StepDone);
    stmt->reset();
    return ok;
}

// The statement's masks are bound already
bool insertNames(SqliteStmt *stmt, const QSet<QString> &names)
{
    for (const QString &name : names) {
        stmt->bindText(1, name);

        if (!stepDone(stmt))
            return false;
    }

    return true;
}

}

HostZonesDb::HostZonesDb(const QString &filePath) : m_db(new SqliteDb(filePath)) { }

HostZonesDb::~HostZonesDb() = default;

QString HostZonesDb::filePath() const
{
    return m_db->filePath();
}

void HostZonesDb::setFilePath(const QString &v)
{
    m_db->setFilePath(v);
}

bool HostZonesDb::isOpened() const
{
    return m_db->db() != nullptr;
}

bool HostZonesDb::open()
{
    if (!m_db->open()) {
        qCCritical(LC) << "File open error:" << filePath() << m_db->errorMessage();
        close();
        return false;
    }

    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/dns/migrations",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .importOldData = false,
    };

    if (!m_db->migrate(opt)) {
        qCCritical(LC) << "Migration error" << filePath();
        close();
        return false;
    }

    loadPatterns();

    return true;
}

void HostZonesDb::close()
{
    m_db->close();

    m_enabledMask = 0;
    m_patterns.clear();
}

quint32 HostZonesDb::zonesMask() const
{
    quint32 zonesMask = 0;

    SqliteStmt *stmt = m_db->stmt(sqlSelectZoneIds);

    while (stmt->step() == SqliteStmt::StepRow) {
        zonesMask |= Zone::zoneBit(stmt->columnInt(0));
    }
    stmt->reset();

    return zonesMask;
}

bool HostZonesDb::isZoneActual(int zoneId, const QString &checksum, bool subdomains) const
{
    SqliteStmt *stmt = m_db->stmt(sqlSelectZone);

    stmt->bindInt(1, zoneId);
    stmt->bindText(2, checksum);
    stmt->bindBool(3, subdomains);

    const bool found = (stmt->step() == SqliteStmt::StepRow);
    stmt->reset();

    return found;
}

bool HostZonesDb::setZone(int zoneId, const QString &checksum, const HostRange &hostRange)
{
    m_db->beginWriteTransaction();

    const bool ok = clearZones(Zone::zoneBit(zoneId)) && insertZone(zoneId, hostRange)
            && saveZone(zoneId, checksum, hostRange.withSubdomains());

    if (!ok) {
        qCWarning(LC) << "Zone save error:" << zoneId << m_db->errorMessage();
    }

    return endWrite(ok);
}

bool HostZonesDb::removeZone(int zoneId)
{
    return removeZones(Zone::zoneBit(zoneId));
}

bool HostZonesDb::retainZones(quint32 zonesMask)
{
    return removeZones(~zonesMask);
}

quint32 HostZonesDb::match(QStringView name, quint32 zonesMask) const
{
    zonesMask &= m_enabledMask;
    if (zonesMask == 0)
        return 0;

    const QByteArray key = HostRange::normalizedKey(name);

    const quint32 namesMask = matchNames(key, zonesMask);
    if (namesMask == zonesMask)
        return namesMask;

    return namesMask | matchPatterns(key, zonesMask & ~namesMask);
}

// Returns whether any Zone was listed
bool HostZonesDb::removeZones(quint32 zonesMask)
{
    zonesMask &= this->zonesMask();
    if (zonesMask == 0)
        return false;

    m_db->beginWriteTransaction();

    const bool ok = clearZones(zonesMask);

    if (!ok) {
        qCWarning(LC) << "Zones remove error:" << zonesMask << m_db->errorMessage();
    }

    return endWrite(ok);
}

bool HostZonesDb::execInt(const char *sql, qint64 v)
{
    SqliteStmt *stmt = m_db->stmt(sql);

    stmt->bindInt64(1, v);

    return stepDone(stmt);
}

bool HostZonesDb::endWrite(bool ok)
{
    ok = m_db->endTransaction(ok);

    // The written pages are kept in the DB's cache
    m_db->shrinkMemory();

    loadPatterns();

    return ok;
}

bool HostZonesDb::clearZones(quint32 zonesMask)
{
    return execInt(sqlClearHosts, zonesMask) && execInt(sqlClearPatterns, zonesMask)
            && m_db->execute(sqlDeleteEmptyNames) && deleteZones(zonesMask);
}

bool HostZonesDb::deleteZones(quint32 zonesMask)
{
    const int zoneIdMax = ConfUtil::zoneMaxCount();
    for (int zoneId = 1; zoneId <= zoneIdMax; ++zoneId) {
        if ((zonesMask & Zone::zoneBit(zoneId)) == 0)
            continue;

        if (!execInt(sqlDeleteZone, zoneId))
            return false;
    }

    return true;
}

bool HostZonesDb::insertZone(int zoneId, const HostRange &hostRange)
{
    const quint32 zoneBit = Zone::zoneBit(zoneId);

    SqliteStmt *hostStmt = m_db->stmt(sqlUpsertHost);

    hostStmt->bindInt64(2, zoneBit);
    hostStmt->bindInt64(3, 0);

    if (!insertNames(hostStmt, hostRange.names()))
        return false;

    hostStmt->bindInt64(2, 0);
    hostStmt->bindInt64(3, zoneBit);

    if (!insertNames(hostStmt, hostRange.wildcardNames()))
        return false;

    SqliteStmt *patternStmt = m_db->stmt(sqlUpsertPattern);

    patternStmt->bindInt64(2, zoneBit);

    return insertNames(patternStmt, hostRange.patternNames());
}

bool HostZonesDb::saveZone(int zoneId, const QString &checksum, bool subdomains)
{
    SqliteStmt *stmt = m_db->stmt(sqlUpsertZone);

    stmt->bindInt(1, zoneId);
    stmt->bindText(2, checksum);
    stmt->bindBool(3, subdomains);

    return stepDone(stmt);
}

void HostZonesDb::loadPatterns()
{
    m_patterns.clear();

    SqliteStmt *stmt = m_db->stmt(sqlSelectPatterns);

    while (stmt->step() == SqliteStmt::StepRow) {
        m_patterns.append({ quint32(stmt->columnInt64(1)), stmt->columnText(0).toLatin1() });
    }
    stmt->reset();
}

HostZonesDb::HostMasks HostZonesDb::selectHost(QByteArrayView key) const
{
    HostMasks masks;

    SqliteStmt *stmt = m_db->stmt(sqlSelectHost);

    stmt->bindTextView(1, key);

    if (stmt->step() == SqliteStmt::StepRow) {
        masks.nameMask = quint32(stmt->columnInt64(0));
        masks.subMask = quint32(stmt->columnInt64(1));
    }
    stmt->reset();

    return masks;
}

quint32 HostZonesDb::matchNames(const QByteArray &key, quint32 zonesMask) const
{
    quint32 matchedMask = 0;

    // A statement with a transaction of its own takes several times longer
    m_db->beginTransaction();

    HostRange::walkName(
            key,
            [&](QByteArrayView nameKey) { matchedMask = selectHost(nameKey).nameMask & zonesMask; },
            [&](QByteArrayView parentKey) {
                if (matchedMask != zonesMask) {
                    matchedMask |= selectHost(parentKey).subMask & zonesMask;
                }
            });

    m_db->commitTransaction();

    return matchedMask;
}

quint32 HostZonesDb::matchPatterns(const QByteArray &key, quint32 zonesMask) const
{
    quint32 matchedMask = 0;

    for (const Pattern &pattern : m_patterns) {
        const quint32 patternMask = pattern.zonesMask & zonesMask;
        if (patternMask == 0)
            continue;

        if (HostPattern::match(pattern.name, key)) {
            matchedMask |= patternMask;
            zonesMask &= ~patternMask;

            if (zonesMask == 0)
                break;
        }
    }

    return matchedMask;
}
