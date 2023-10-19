#ifndef DRIVELISTMANAGER_H
#define DRIVELISTMANAGER_H

#include <QObject>

#include <util/ioc/iocservice.h>

class DriveListManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DriveListManager(QObject *parent = nullptr);

    void initialize();

signals:
    void driveMaskChanged(quint32 driveMask);

public slots:
    void onDriveListChanged();

private:
    quint32 m_driveMask = 0;
};

#endif // DRIVELISTMANAGER_H
