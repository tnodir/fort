#ifndef HOSTZONESSYNC_H
#define HOSTZONESSYNC_H

#include <util/worker/workermanager.h>

struct HostZonesSource;

// Loads the Hosts Zones' names into the DB by a connection of its own on a worker thread: the
// matches on the service's thread meanwhile read the DB's last commit
class HostZonesSync : public WorkerManager
{
    Q_OBJECT

public:
    explicit HostZonesSync(QObject *parent = nullptr);

    QString workerName() const override { return "HostZonesWorker"; }

signals:
    // Whether the names to match changed; the first error of the DB's opening or writing
    void synced(bool changed, const QString &errorMessage);

public slots:
    void sync(const QString &filePath, const HostZonesSource &source);
};

#endif // HOSTZONESSYNC_H
