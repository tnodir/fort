#ifndef TASKINFOZONEDOWNLOADER_H
#define TASKINFOZONEDOWNLOADER_H

#include <QSet>

#include "taskinfo.h"

QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(TaskZoneDownloader)
QT_FORWARD_DECLARE_CLASS(ZoneListModel)

class TaskInfoZoneDownloader : public TaskInfo
{
    Q_OBJECT

public:
    explicit TaskInfoZoneDownloader(FortManager *fortManager,
                                    QObject *parent = nullptr);

    TaskZoneDownloader *zoneDownloader() const;
    ZoneListModel *zoneListModel() const;

public slots:
    bool processResult(bool success) override;

protected slots:
    void setupTaskWorker() override;

    void handleFinished(bool success) override;

    void processSubResult(bool success);

private:
    void removeOrphanCacheFiles();

    QString cachePath() const;

private:
    bool m_success = false;
    int m_zoneIndex = 0;

    QSet<int> m_zoneIdSet;
};

#endif // TASKINFOZONEDOWNLOADER_H
