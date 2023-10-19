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
    ~ServiceManager() override;
    CLASS_DELETE_COPY_MOVE(ServiceManager)

    void setUp() override;
    void tearDown() override;

    bool controlEnabled() const { return m_controlEnabled; }
    void setControlEnabled(bool v);

    void initialize(qintptr hstatus) override;

    const wchar_t *serviceName() const override;

    void processControl(quint32 code, quint32 eventType) override;

signals:
    void pauseRequested();
    void continueRequested();
    void driveListChanged();

protected:
    void setupControlManager();
    void setupConfManager();

    bool acceptStop() const override { return controlEnabled(); }
    bool acceptPauseContinue() const override { return controlEnabled(); }

private:
    void setupByConf(const IniOptions &ini);

private:
    bool m_controlEnabled = false;
};

#endif // SERVICEMANAGER_H
