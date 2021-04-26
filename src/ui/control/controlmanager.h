#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include <QObject>
#include <QVariant>

#include "../util/classhelpers.h"
#include "control.h"

QT_FORWARD_DECLARE_CLASS(QLocalServer)
QT_FORWARD_DECLARE_CLASS(QLocalSocket)

class ControlWorker;
class FortManager;
class FortSettings;

class ControlManager : public QObject
{
    Q_OBJECT

public:
    explicit ControlManager(FortSettings *settings, QObject *parent = nullptr);
    ~ControlManager() override;
    CLASS_DELETE_COPY_MOVE(ControlManager)

    FortSettings *settings() const { return m_settings; }
    FortManager *fortManager() const { return m_fortManager; }

    const QList<ControlWorker *> &clients() const { return m_clients; }

    bool isClient() const;

    bool listen(FortManager *fortManager);
    bool postCommand();

private slots:
    void onNewConnection();

    bool processRequest(Control::Command command, const QVariantList &args);

private:
    bool processCommand(Control::Command command, const QVariantList &args, QString &errorMessage);

    void abort();

    static QString getServerName(bool isService);

private:
    FortSettings *m_settings = nullptr;
    FortManager *m_fortManager = nullptr;

    QLocalServer *m_server = nullptr;

    QList<ControlWorker *> m_clients;
};

#endif // CONTROLMANAGER_H
