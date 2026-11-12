#ifndef DNSSERVERSTEST_H
#define DNSSERVERSTEST_H

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>
#include <QTimer>

#include <memory>

class DnsAnswer;
class DnsResolver;

// The test's result of an upstream server
struct DnsServerResult
{
    enum Status : qint8 {
        StatusNone = 0, // not answered yet
        StatusAnswered,
        StatusFailed, // the resolver's error
        StatusTimeout,
        StatusInvalid, // the server's text isn't valid
        StatusSkipped, // not asked: another server is invalid
    };

    // The server's answer is NOERROR
    bool isNoError() const;

    Status status = StatusNone;
    quint8 rcode = 0;
    int msecs = 0;
    QString error;
};

// Asks each upstream server for a name by a resolver of its own, in parallel
class DnsServersTest : public QObject
{
    Q_OBJECT

public:
    // The servers as of DnsUtil::splitServers(): the empty list is of the adapters' DNS servers,
    // tested as one empty server
    explicit DnsServersTest(const QStringList &servers, QObject *parent = nullptr);
    ~DnsServersTest() override;

    const QStringList &servers() const { return m_servers; }

    const QList<DnsServerResult> &results() const { return m_results; }

    bool isFinished() const { return m_resultsCount >= m_results.size(); }

    // Each server's answer is NOERROR
    bool isAllNoError() const;

    using ResultTextFunc = QString (*)(const DnsServerResult &result);

    // The "server: text" lines of the results with a text, the adapters' DNS servers by the name
    QStringList resultLines(ResultTextFunc resultText, const QString &adaptersName) const;

    void setTimeout(int msecs) { m_timer.setInterval(msecs); }

    // With an invalid server no server is asked
    virtual void start();

    void setResult(int index, const DnsServerResult &result);

signals:
    void resultReady(int index);
    void finished();

private:
    bool openResolvers();

    void processAnswer(int index, const DnsAnswer &answer);
    void processTimeout();

private:
    int m_resultsCount = 0;

    QElapsedTimer m_elapsedTimer;
    QTimer m_timer;

    QStringList m_servers;
    QList<DnsServerResult> m_results;

    std::vector<std::unique_ptr<DnsResolver>> m_resolvers;
};

#endif // DNSSERVERSTEST_H
