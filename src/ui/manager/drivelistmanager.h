#ifndef DRIVELISTMANAGER_H
#define DRIVELISTMANAGER_H

#include <QObject>

#include <util/ioc/iocservice.h>
#include <util/triggertimer.h>

class DriveListManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DriveListManager(QObject *parent = nullptr);

    void setUp() override;

signals:
    void driveMaskChanged(quint32 addedMask, quint32 removedMask);

public slots:
    virtual void onDriveListChanged();

protected slots:
    virtual void populateDriveMask();

private:
    void setDriveMask(quint32 driveMask);

private:
    bool m_isUserAdmin = false;

    quint32 m_driveMask = 0;

    TriggerTimer m_driveListTimer;
};

#endif // DRIVELISTMANAGER_H
