#ifndef DRIVELISTMANAGER_H
#define DRIVELISTMANAGER_H

#include <QObject>

#include <util/ioc/iocservice.h>

class DriveListManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DriveListManager(QObject *parent = nullptr);

    void setUp() override;

signals:
    void driveMaskChanged(quint32 addedMask, quint32 removedMask);

public slots:
    void onDriveListChanged();

private:
    quint32 m_driveMask = 0;
};

#endif // DRIVELISTMANAGER_H
