#ifndef DNSETWLISTENER_H
#define DNSETWLISTENER_H

#include <QObject>

#include <memory>

#include "dnsclientanswer.h"

QT_FORWARD_DECLARE_CLASS(QThread)

struct _EVENT_RECORD;

// Receives the DNS Client's answers to the processes' queries by the ETW events of the
// Microsoft-Windows-DNS-Client provider, on the trace's thread
class DnsEtwListener : public QObject
{
    Q_OBJECT

public:
    explicit DnsEtwListener(QObject *parent = nullptr);
    ~DnsEtwListener() override;
    Q_DISABLE_COPY_MOVE(DnsEtwListener)

    bool isOpened() const { return m_traceThread != nullptr; }

    bool open();
    void close();

signals:
    void answerReceived(const DnsClientAnswer &answer);

private:
    bool startSession();
    void stopSession();

    bool openTrace();
    void closeTrace();

    void processEvent(_EVENT_RECORD *event);

    // Called on the trace's thread
    static void __stdcall onEventRecord(_EVENT_RECORD *event);

private:
    quint64 m_sessionHandle = 0;
    quint64 m_traceHandle = 0;

    std::unique_ptr<QThread> m_traceThread;
};

#endif // DNSETWLISTENER_H
