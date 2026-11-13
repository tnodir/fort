#include "dnsserverstest.h"

#include <algorithm>

#include <ares.h>

#include "dnshttpsserver.h"
#include "dnsresolver.h"
#include "dnsutil.h"

namespace {

inline constexpr char testName[] = "example.com";

inline constexpr int testTimeout = 5 * 1000; // msecs: over DnsHttpsResolver's request timeout

QStringList serverList(const QString &server)
{
    if (server.isEmpty())
        return {};

    return { server };
}

}

bool DnsServerResult::isNoError() const
{
    return status == StatusAnswered && rcode == ARES_RCODE_NOERROR;
}

DnsServersTest::DnsServersTest(const QStringList &servers, QObject *parent) :
    QObject(parent),
    m_servers(servers.isEmpty() ? QStringList { QString() } : servers),
    m_results(m_servers.size())
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(testTimeout);

    connect(&m_timer, &QTimer::timeout, this, &DnsServersTest::processTimeout);
}

DnsServersTest::~DnsServersTest() = default;

bool DnsServersTest::isAllNoError() const
{
    return std::all_of(m_results.begin(), m_results.end(),
            [](const DnsServerResult &result) { return result.isNoError(); });
}

QStringList DnsServersTest::resultLines(
        ResultTextFunc resultText, const QString &adaptersName) const
{
    QStringList lines;

    for (int i = 0; i < m_results.size(); ++i) {
        const QString text = resultText(m_results.at(i));
        if (text.isEmpty())
            continue;

        const QString &server = m_servers.at(i);

        lines.append((server.isEmpty() ? adaptersName : server) + ": " + text);
    }

    return lines;
}

void DnsServersTest::start()
{
    if (!openResolvers())
        return;

    DnsRequest request;
    request.query.parse(DnsUtil::writeQuery(/*id=*/1, testName));

    startTimeout();

    for (const auto &resolver : m_resolvers) {
        resolver->resolve(request);
    }
}

void DnsServersTest::setResult(int index, const DnsServerResult &result)
{
    // The late result after the timeout, the resolver's repeated query
    if (m_results.at(index).status != DnsServerResult::StatusNone)
        return;

    m_results[index] = result;
    ++m_resultsCount;

    emit resultReady(index);

    if (isFinished()) {
        m_timer.stop();

        emit finished();
    }
}

bool DnsServersTest::checkLoopQuery(const DnsQuery &query)
{
    const auto it = std::find_if(m_resolvers.begin(), m_resolvers.end(),
            [&](const auto &resolver) { return resolver->isOwnQuery(query); });
    if (it == m_resolvers.end())
        return false;

    // The resolvers are of the servers when started
    const int index = int(it - m_resolvers.begin());

    setResult(index, { .status = DnsServerResult::StatusLoop });

    return true;
}

bool DnsServersTest::openResolvers()
{
    // The forwarder can't ask them
    if (DnsHttpsServer::isMixedServers(m_servers)) {
        setNotAskedResults(DnsServerResult::StatusMixed);
        return false;
    }

    QList<int> invalidIndexes;

    for (int i = 0; i < m_servers.size(); ++i) {
        const QStringList servers = serverList(m_servers.at(i));

        auto resolver = DnsResolver::create(servers);

        if (!resolver->open(servers)) {
            invalidIndexes.append(i);
            continue;
        }

        connect(resolver.get(), &DnsResolver::resolved, this,
                [=, this](const DnsRequest & /*request*/, const DnsAnswer &answer) {
                    processAnswer(i, answer);
                });

        m_resolvers.push_back(std::move(resolver));
    }

    if (invalidIndexes.isEmpty())
        return true;

    m_resolvers.clear();

    setNotAskedResults(DnsServerResult::StatusSkipped, invalidIndexes);

    return false;
}

void DnsServersTest::setNotAskedResults(
        DnsServerResult::Status status, const QList<int> &invalidIndexes)
{
    for (int i = 0; i < m_servers.size(); ++i) {
        setResult(i,
                { .status = invalidIndexes.contains(i) ? DnsServerResult::StatusInvalid : status });
    }
}

void DnsServersTest::processAnswer(int index, const DnsAnswer &answer)
{
    const bool failed = !answer.error().isEmpty();

    setResult(index,
            {
                    .status = failed ? DnsServerResult::StatusFailed
                                     : DnsServerResult::StatusAnswered,
                    .rcode = answer.rcode(),
                    .msecs = int(m_elapsedTimer.elapsed()),
                    .error = answer.error(),
            });
}

void DnsServersTest::startTimeout()
{
    m_elapsedTimer.start();
    m_timer.start();
}

void DnsServersTest::processTimeout()
{
    const int msecs = int(m_elapsedTimer.elapsed());

    for (int i = 0; i < m_results.size(); ++i) {
        if (m_results.at(i).status == DnsServerResult::StatusNone) {
            setResult(i, { .status = DnsServerResult::StatusTimeout, .msecs = msecs });
        }
    }
}
