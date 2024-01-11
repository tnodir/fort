#ifndef DBERRORMANAGER_H
#define DBERRORMANAGER_H

#include <QObject>

#include <util/ioc/iocservice.h>

QT_FORWARD_DECLARE_CLASS(QTimer)

class DbErrorManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DbErrorManager(QObject *parent = nullptr);

    void setUp() override;

public slots:
    void startPolling();

private slots:
    void checkDriveList();

private:
    void setupDriveMask();

    void setupPollingTimer();

private:
    bool m_polling = false;

    quint32 m_driveMask = 0;

    QTimer *m_pollingTimer = nullptr;
};

#endif // DBERRORMANAGER_H
