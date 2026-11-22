#ifndef TASKINFOZONEDOWNLOADER_H
#define TASKINFOZONEDOWNLOADER_H

#include <QByteArray>
#include <QList>
#include <QString>

#include "taskinfo.h"

class HostRange;
class HostZonesDb;
class TaskZoneDownloader;

struct ZonesData
{
    quint32 zonesMask = 0; // all Zones
    quint32 hostsMask = 0; // the Hosts Zones
    quint32 dataZonesMask = 0;
    quint32 enabledMask = 0;
    quint32 dataSize = 0;

    QList<QByteArray> zonesData;
};

// A Hosts Zone's cache file to load its names from
struct HostZoneSource
{
    bool subdomains = false;

    int zoneId = 0;

    QString binChecksum;
    QString cacheFilePath;
};

struct HostZonesSource
{
    quint32 hostsMask = 0; // all Hosts Zones
    quint32 enabledMask = 0;

    QList<HostZoneSource> zones;
};

class TaskInfoZoneDownloader : public TaskInfo
{
    Q_OBJECT

public:
    explicit TaskInfoZoneDownloader(TaskManager &taskManager);

    const QStringList &zoneNames() const { return m_zoneNames; }

    const ZonesData &zones() const { return m_zones; }

    TaskZoneDownloader *zoneDownloader() const;

    void initialize() override;

    // Updates the Zones of the mask, loading the others from the cache: after the current run
    void runZones(quint32 zonesMask);

    // Loads the Zones' addresses from the cache
    ZonesData loadZonesData() const;

    // The Hosts Zones' cache files to load their names from
    HostZonesSource hostZonesSource() const;

    // Loads the changed Hosts Zone's names from its cache file into the DB: called by a worker
    // thread. Returns whether the names to match changed.
    static bool syncHostZone(HostZonesDb &db, const HostZoneSource &zone, HostRange &hostRange);

public slots:
    // Runs all Zones: after the current run of some Zones
    void run() override;

    bool processResult(bool success) override;

    bool saveZoneAsText(const QString &filePath, int zoneIndex) const;

protected slots:
    void setupTaskWorker() override;

    void handleFinished(bool success) override;

    void processSubResult(bool success);
    void clearSubResults();

private:
    bool isAllZonesRun() const { return m_runZonesMask == allZonesMask; }
    void addPendingZones(quint32 zonesMask);

    bool setupNextTaskWorker();
    int nextZoneIndex(int zoneIndex) const;
    void finishZones();
    void finishRunZones(bool allZones);
    void runPendingZones(bool aborted);
    void setupTaskWorkerByZone(TaskZoneDownloader *worker, int zoneIndex) const;
    void addSubResult(TaskZoneDownloader *worker, bool success);

    static void addZoneData(ZonesData &zones, const TaskZoneDownloader &worker);

    static void insertZoneId(quint32 &zonesMask, int zoneId);
    static bool containsZoneId(quint32 zonesMask, int zoneId);

    void loadZones();

    void emitZonesUpdated();

    void removeOrphanCacheFiles();

    QString cachePath() const;

private:
    static constexpr quint32 allZonesMask = ~quint32(0);

    bool m_success = false;
    int m_zoneIndex = 0;

    quint32 m_runZonesMask = allZonesMask;
    quint32 m_pendingZonesMask = 0;

    QStringList m_zoneNames;

    ZonesData m_zones;
};

#endif // TASKINFOZONEDOWNLOADER_H
