#ifndef SERVICEINFOMONITOR_H
#define SERVICEINFOMONITOR_H

#include <QObject>

#include "serviceinfo.h"

class ServiceInfoMonitor : public QObject
{
    Q_OBJECT

public:
    explicit ServiceInfoMonitor(quint32 processId, const QString &name,
            void *managerHandle = nullptr, QObject *parent = nullptr);
    ~ServiceInfoMonitor() override;

    bool running() const { return m_running; }
    void setRunning(bool v) { m_running = v; }

    quint32 processId() const { return m_processId; }
    void setProcessId(quint32 v) { m_processId = v; }

    QString name() const { return m_name; }

    QVector<char> &notifyBuffer() { return m_notifyBuffer; }

signals:
    void stateChanged(ServiceInfo::State state);
    void errorOccurred();

public slots:
    void terminate();

    void requestReopenService();
    void requestStartNotifier();

private:
    void openService(void *managerHandle = nullptr);
    void closeService();
    void reopenService();

    void startNotifier();

private:
    bool m_terminated : 1;
    bool m_isReopening : 1;
    bool m_reopenServiceRequested : 1;
    bool m_startNotifierRequested : 1;
    bool m_running : 1;

    quint32 m_processId = 0;

    const QString m_name;

    void *m_serviceHandle = nullptr;
    QVector<char> m_notifyBuffer;
};

#endif // SERVICEINFOMONITOR_H
