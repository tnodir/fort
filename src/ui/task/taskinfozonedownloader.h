#ifndef TASKINFOZONEDOWNLOADER_H
#define TASKINFOZONEDOWNLOADER_H

#include <QByteArray>

#include "taskinfo.h"

class TaskZoneDownloader;
class ZoneListModel;

class TaskInfoZoneDownloader : public TaskInfo
{
    Q_OBJECT

public:
    explicit TaskInfoZoneDownloader(TaskManager &taskManager);

    quint32 dataZonesMask() const { return m_dataZonesMask; }
    quint32 enabledMask() const { return m_enabledMask; }
    quint32 dataSize() const { return m_dataSize; }

    const QStringList &zoneNames() const { return m_zoneNames; }
    const QList<QByteArray> &zonesData() const { return m_zonesData; }

    TaskZoneDownloader *zoneDownloader() const;
    ZoneListModel *zoneListModel() const;

    void initialize() override;

public slots:
    bool processResult(bool success) override;

    bool saveZoneAsText(const QString &filePath, int zoneIndex);

protected slots:
    void setupTaskWorker() override;

    void handleFinished(bool success) override;

    void processSubResult(bool success);
    void clearSubResults();

private:
    void setupNextTaskWorker();
    void setupTaskWorkerByZone(TaskZoneDownloader *worker);
    void addSubResult(TaskZoneDownloader *worker, bool success);

    void insertZoneId(quint32 &zonesMask, int zoneId);
    bool containsZoneId(quint32 zonesMask, int zoneId) const;

    void loadZones();

    void emitZonesUpdated();

    void removeOrphanCacheFiles();

    QString cachePath() const;

private:
    bool m_success = false;
    int m_zoneIndex = 0;
    quint32 m_zonesMask = 0;

    quint32 m_dataZonesMask = 0;
    quint32 m_enabledMask = 0;
    quint32 m_dataSize = 0;

    QStringList m_zoneNames;
    QList<QByteArray> m_zonesData;
};

#endif // TASKINFOZONEDOWNLOADER_H
