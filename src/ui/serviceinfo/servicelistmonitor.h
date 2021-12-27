#ifndef SERVICELISTMONITOR_H
#define SERVICELISTMONITOR_H

#include <QObject>

class ServiceListMonitor : public QObject
{
    Q_OBJECT

public:
    explicit ServiceListMonitor(void *managerHandle = nullptr, QObject *parent = nullptr);
    ~ServiceListMonitor() override;

    QVector<char> &notifyBuffer() { return m_notifyBuffer; }

signals:
    void serviceCreated(const QStringList &nameList);

public slots:
    void terminate();

    void requestStartNotifier();

private:
    void openManager();
    void closeManager();
    void reopenManager();

    void startNotifier();

private:
    bool m_terminated : 1;
    bool m_isReopening : 1;
    bool m_startNotifierRequested : 1;

    void *m_managerHandle = nullptr;
    QVector<char> m_notifyBuffer;
};

#endif // SERVICELISTMONITOR_H
