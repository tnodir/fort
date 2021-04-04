#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

#include "../util/classhelpers.h"
#include "control.h"

QT_FORWARD_DECLARE_CLASS(QLocalServer)
QT_FORWARD_DECLARE_CLASS(QLocalSocket)

class FortManager;
class FortSettings;

class ControlManager : public QObject
{
    Q_OBJECT

public:
    explicit ControlManager(FortSettings *settings, QObject *parent = nullptr);
    ~ControlManager() override;
    CLASS_DELETE_COPY_MOVE(ControlManager)

    bool isClient() const;

    bool listen(FortManager *fortManager);
    bool postCommand();

    FortSettings *settings() const { return m_settings; }
    FortManager *fortManager() const { return m_fortManager; }

private slots:
    void onNewConnection();

    bool processRequest(Control::Command command, const QStringList &args);

private:
    bool processCommand(Control::Command command, const QStringList &args, QString &errorMessage);

    void abort();

    static QString getServerName(bool isService);

private:
    FortSettings *m_settings = nullptr;
    FortManager *m_fortManager = nullptr;

    QLocalServer *m_server = nullptr;
};

#endif // CONTROLMANAGER_H
