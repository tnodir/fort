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

    bool isCommandClient() const;

    ControlWorker *newServiceClient(QObject *parent = nullptr) const;

    bool listen();
    void close();

    void closeAllClients();

    bool processCommandClient();
    bool postCommand(Control::Command command, const QVariantList &args);

private slots:
    bool connectToAnyServer(ControlWorker &w);

    void onNewConnection();
    void onDisconnected();

    bool processRequest(Control::Command command, const QVariantList &args);

private:
    enum ProgAction : quint32 {
        ProgActionNone = 0,
        ProgActionAdd = (1 << 0),
        ProgActionDel = (1 << 1),
        ProgActionAllow = (1 << 2),
        ProgActionBlock = (1 << 3),
        ProgActionKill = (1 << 4),
    };

    bool processCommand(const ProcessCommandArgs &p);

    bool processCommandHome(const ProcessCommandArgs &p);

    bool processCommandProg(const ProcessCommandArgs &p);
    bool processCommandProgAction(ProgAction progAction, const QString &appPath);
    static bool checkProgActionPassword(ProgAction progAction);
    static ProgAction progActionByText(const QString &commandText);

    static QString getServerName(bool isService = false);

private:
    QLocalServer *m_server = nullptr;

    QList<ControlWorker *> m_clients;
};

#endif // CONTROLMANAGER_H
