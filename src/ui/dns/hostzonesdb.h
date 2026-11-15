#ifndef HOSTZONESDB_H
#define HOSTZONESDB_H

#include <QByteArray>
#include <QByteArrayView>
#include <QString>
#include <QVector>

#include <memory>

#include <util/classhelpers.h>

class HostRange;
class SqliteDb;
class SqliteStmt;

// The Hosts Zones' names in a DB, filled from the Zones' cache files
class HostZonesDb
{
public:
    explicit HostZonesDb(const QString &filePath = {});
    ~HostZonesDb();
    CLASS_DELETE_COPY_MOVE(HostZonesDb)

    QString filePath() const;
    void setFilePath(const QString &v);

    // Only the enabled Zones are matched
    quint32 enabledMask() const { return m_enabledMask; }
    void setEnabledMask(quint32 v) { m_enabledMask = v; }

    bool isOpened() const;

    bool open();
    void close();

    // The Zones having their names in the DB
    quint32 zonesMask() const;

    // Whether the Zone's names are of its cache file and type
    bool isZoneActual(int zoneId, const QString &checksum, bool subdomains) const;

    // Replaces the Zone's names
    bool setZone(int zoneId, const QString &checksum, const HostRange &hostRange);

    // Returns whether the Zone was listed
    bool removeZone(int zoneId);

    // Removes the other Zones; returns whether any was listed
    bool retainZones(quint32 zonesMask);

    // The enabled Zones of the mask listing the name by itself, by a "*." wildcard of its parent
    // domain or by another mask
    quint32 match(QStringView name, quint32 zonesMask) const;

private:
    struct HostMasks
    {
        quint32 nameMask = 0; // the Zones listing the name itself
        quint32 subMask = 0; // the Zones listing its subdomains
    };

    struct Pattern
    {
        quint32 zonesMask = 0;
        QByteArray name;
    };

    bool removeZones(quint32 zonesMask);

    bool execInt(const char *sql, qint64 v);

    bool endWrite(bool ok);

    bool clearZones(quint32 zonesMask);
    bool deleteZones(quint32 zonesMask);
    bool insertZone(int zoneId, const HostRange &hostRange);
    bool saveZone(int zoneId, const QString &checksum, bool subdomains);

    void loadPatterns();

    HostMasks selectHost(QByteArrayView key) const;

    // The Zones of the mask listing the name by itself or by a "*." wildcard of its parent domain
    quint32 matchNames(const QByteArray &key, quint32 zonesMask) const;
    // The Zones of the mask listing the name by a pattern
    quint32 matchPatterns(const QByteArray &key, quint32 zonesMask) const;

private:
    quint32 m_enabledMask = 0;

    std::unique_ptr<SqliteDb> m_db;

    QVector<Pattern> m_patterns; // the names with other masks: HostPattern
};

#endif // HOSTZONESDB_H
