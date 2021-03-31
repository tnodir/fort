#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

#include "../util/classhelpers.h"

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

    bool isClient() const;

    bool listen(FortManager *fortManager);
    bool post();

    FortSettings *settings() const { return m_settings; }
    FortManager *fortManager() const { return m_fortManager; }

private slots:
    bool processRequest(const QString &command, const QStringList &args);

private:
    bool processCommand(const QString &command, const QStringList &args, QString &errorMessage);

    void setupWorker();

    void abort();

private:
    FortSettings *m_settings = nullptr;
    FortManager *m_fortManager = nullptr;
    ControlWorker *m_worker = nullptr;

    QSystemSemaphore m_semaphore;
    QSharedMemory m_sharedMemory;
};

#endif // CONTROLMANAGER_H
