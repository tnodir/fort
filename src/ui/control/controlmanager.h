#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include <QObject>
#include <QVariant>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>

#include "control.h"

QT_FORWARD_DECLARE_CLASS(QLocalServer)

class ControlManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit ControlManager(QObject *parent = nullptr);
    ~ControlManager() override;
    CLASS_DELETE_COPY_MOVE(ControlManager)

    const QList<ControlWorker *> &clients() const { return m_clients; }

    void setUp() override;

    ControlWorker *newServiceClient(QObject *parent = nullptr) const;

    bool listen();
    void close();

    void closeAllClients();

    bool processCommandClient(ProcessCommandResult &r);

    bool postCommand(
            Control::Command command, const QVariantList &args, ProcessCommandResult *r = nullptr);

private slots:
    bool connectToAnyServer(ControlWorker &w);

    void onNewConnection();
    void onDisconnected();

    bool processRequest(Control::Command command, const QVariantList &args);

private:
    static QString getServerName(bool isService = false);

private:
    QLocalServer *m_server = nullptr;

    QList<ControlWorker *> m_clients;
};

#endif // CONTROLMANAGER_H
