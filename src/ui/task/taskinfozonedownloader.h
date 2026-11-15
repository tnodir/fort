#ifndef TASKINFOZONEDOWNLOADER_H
#define TASKINFOZONEDOWNLOADER_H

#include <QByteArray>

#include "taskinfo.h"

class HostRange;
class HostZonesDb;
class TaskZoneDownloader;

struct ZonesData
{
    quint32 zonesMask = 0; // all Zones
    quint32 dataZonesMask = 0;
    quint32 enabledMask = 0;
    quint32 dataSize = 0;

    QList<QByteArray> zonesData;
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

    // Loads the Zones' addresses from the cache
    ZonesData loadZonesData() const;

    // Loads the changed Hosts Zones' names from the cache into the DB.
    // Returns whether the names to match changed.
    bool syncHostZones(HostZonesDb &db) const;

public slots:
    bool processResult(bool success) override;

    bool saveZoneAsText(const QString &filePath, int zoneIndex) const;

protected slots:
    void setupTaskWorker() override;

    void handleFinished(bool success) override;

    void processSubResult(bool success);
    void clearSubResults();

private:
    void setupNextTaskWorker();
    void setupTaskWorkerByZone(TaskZoneDownloader *worker, int zoneIndex) const;
    void addSubResult(TaskZoneDownloader *worker, bool success);

    static bool syncHostZone(HostZonesDb &db, TaskZoneDownloader &worker, HostRange &hostRange);

    static void addZoneData(ZonesData &zones, const TaskZoneDownloader &worker);

    static void insertZoneId(quint32 &zonesMask, int zoneId);
    static bool containsZoneId(quint32 zonesMask, int zoneId);

    void loadZones();

    void emitZonesUpdated();

    void removeOrphanCacheFiles();

    QString cachePath() const;

private:
    bool m_success = false;
    int m_zoneIndex = 0;

    QStringList m_zoneNames;

    ZonesData m_zones;
};

#endif // TASKINFOZONEDOWNLOADER_H
