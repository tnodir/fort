#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

#include "../util/classhelpers.h"

class ControlWorker;
class FortManager;

class ControlManager : public QObject
{
    Q_OBJECT

public:
    explicit ControlManager(
            const QString &globalName, const QString &command, QObject *parent = nullptr);
    ~ControlManager() override;
    CLASS_DELETE_COPY_MOVE(ControlManager)

    bool isClient() const { return m_isClient; }

    bool listen(FortManager *fortManager);
    bool post(const QStringList &args);

signals:

public slots:

private slots:
    void processRequest(const QString &command, const QStringList &args);

private:
    bool processCommand(const QString &command, const QStringList &args, QString &errorMessage);

    void setupWorker();

    void abort();

private:
    bool m_isClient = false;

    FortManager *m_fortManager = nullptr;
    ControlWorker *m_worker = nullptr;

    QString m_command;

    QSystemSemaphore m_semaphore;
    QSharedMemory m_sharedMemory;
};

#endif // CONTROLMANAGER_H
