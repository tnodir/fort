#ifndef DBERRORMANAGER_H
#define DBERRORMANAGER_H

#include <QObject>

#include <util/ioc/iocservice.h>

class DbErrorManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DbErrorManager(QObject *parent = nullptr);

    void setUp() override;

public slots:
    void onDbIoError();

private slots:
    void checkDriveList();

private:
    void setupDriveMask();

private:
    bool m_polling = false;

    quint32 m_driveMask = 0;
};

#endif // DBERRORMANAGER_H
