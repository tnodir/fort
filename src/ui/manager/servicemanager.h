#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>
#include <util/service/servicemanageriface.h>

class IniOptions;

class ServiceManager : public QObject, public ServiceManagerIface, public IocService
{
    Q_OBJECT

public:
    explicit ServiceManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ServiceManager)

    void setUp() override;
    void tearDown() override;

    bool controlEnabled() const { return m_controlEnabled; }
    void setControlEnabled(bool v);

    bool acceptStop() const override { return controlEnabled(); }
    bool acceptPauseContinue() const override { return controlEnabled(); }

    void initialize(qintptr hstatus) override;

    const wchar_t *serviceName() const override;

    void processControl(quint32 code, quint32 eventType) override;

    void restart();

signals:
    void pauseRequested();
    void continueRequested();
    void stopRestartingRequested(bool restarting);
    void driveListChanged();

protected:
    void setupControlManager();
    void setupConfManager();

private:
    void setupByConf(const IniOptions &ini);

private:
    bool m_controlEnabled = false;
};

#endif // SERVICEMANAGER_H
