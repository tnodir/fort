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
    CLASS_DELETE_COPY_MOVE(ServiceManager)

    void setUp() override;

    bool controlEnabled() const { return m_controlEnabled.loadRelaxed() != 0; }
    void setControlEnabled(bool v);

    bool stopDriverRequested() const { return m_stopDriverRequested; }

    bool acceptStop() const override { return controlEnabled(); }
    bool acceptPauseContinue() const override { return controlEnabled(); }

    const wchar_t *serviceName() const override;

    void processControl(quint32 code, quint32 eventType) override;

    void restart();

signals:
    void pauseRequested();
    void continueRequested();
    void stopRestartingRequested(bool restarting);

protected:
    void setupControlManager();
    void setupConfManager();

private:
    void setupByConfIni();

private:
    bool m_stopDriverRequested = false;

    QAtomicInt m_controlEnabled = 0; // read by the control handler's thread
};

#endif // SERVICEMANAGER_H
