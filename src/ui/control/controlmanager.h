#ifndef CONTROLMANAGER_H
#define CONTROLMANAGER_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

#include "../util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(ControlWorker)
QT_FORWARD_DECLARE_CLASS(FortManager)

class ControlManager : public QObject
{
    Q_OBJECT

public:
    explicit ControlManager(const QString &globalName,
                            const QString &command,
                            QObject *parent = nullptr);
    ~ControlManager() override;
    CLASS_DELETE_COPY_MOVE(ControlManager)

    bool isClient() const { return m_isClient; }

    bool listen(FortManager *fortManager);
    bool post(const QStringList &args);

signals:

public slots:

private slots:
    void processRequest(const QString &command,
                        const QStringList &args);

private:
    bool processCommand(const QString &command,
                        const QStringList &args,
                        QString &errorMessage);

    void setupWorker();

    void abort();

private:
    bool m_isClient;

    QString m_command;

    FortManager *m_fortManager;
    ControlWorker *m_worker;

    QSystemSemaphore m_semaphore;
    QSharedMemory m_sharedMemory;
};

#endif // CONTROLMANAGER_H
