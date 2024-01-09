#ifndef DRIVELISTMANAGER_H
#define DRIVELISTMANAGER_H

#include <QObject>

#include <util/ioc/iocservice.h>

QT_FORWARD_DECLARE_CLASS(QTimer)

class DriveListManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DriveListManager(QObject *parent = nullptr);

    void initialize();

signals:
    void driveMaskChanged(quint32 addedMask, quint32 removedMask);

public slots:
    void onDriveListChanged();

    void startPolling();

private:
    void setupPollingTimer();

private:
    quint32 m_driveMask = 0;

    QTimer *m_pollingTimer = nullptr;
};

#endif // DRIVELISTMANAGER_H
