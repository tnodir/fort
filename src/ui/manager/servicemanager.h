#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>
#include <util/service/servicemanageriface.h>

class ServiceManager : public QObject, public ServiceManagerIface, public IocService
{
    Q_OBJECT

public:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager() override;
    CLASS_DELETE_COPY_MOVE(ServiceManager)

    void setUp() override;

    const wchar_t *serviceName() const override;

    void processControl(quint32 code) override;

signals:
    void pauseRequested();
    void continueRequested();
};

#endif // SERVICEMANAGER_H
