#ifndef DRIVELISTMANAGER_H
#define DRIVELISTMANAGER_H

#include <QObject>
#include <QPointer>

#include <util/ioc/iocservice.h>
#include <util/triggertimer.h>

#include "drivelistworker.h"

class DriveListManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DriveListManager(QObject *parent = nullptr);

    DriveListWorker *driveListWorker() const { return m_driveListWorker.data(); }

    void setUp() override;
    void tearDown() override;

signals:
    void driveMaskChanged(quint32 addedMask, quint32 removedMask);

public slots:
    virtual void onDriveListChanged();

protected slots:
    virtual void populateDriveMask();

private:
    void setDriveMask(quint32 driveMask);

    void setupWorker();
    void closeWorker();

private:
    bool m_isUserAdmin = false;

    quint32 m_driveMask = 0;

    TriggerTimer m_driveListTimer;

    QPointer<DriveListWorker> m_driveListWorker;
};

#endif // DRIVELISTMANAGER_H
