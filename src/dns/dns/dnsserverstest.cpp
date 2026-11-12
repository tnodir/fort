#include "dnsserverstest.h"

#include <algorithm>

#include <ares.h>

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

    m_elapsedTimer.start();
    m_timer.start();

    for (const auto &resolver : m_resolvers) {
        resolver->resolve(request);
    }
}

void DnsServersTest::setResult(int index, const DnsServerResult &result)
{
    m_results[index] = result;
    ++m_resultsCount;

    emit resultReady(index);

    if (isFinished()) {
        m_timer.stop();

        emit finished();
    }
}

bool DnsServersTest::openResolvers()
{
    QList<int> invalidIndexes;

    for (int i = 0; i < m_servers.size(); ++i) {
        // Of the list's protocol
        auto resolver = DnsResolver::create(m_servers);

        if (!resolver->open(serverList(m_servers.at(i)))) {
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

    for (int i = 0; i < m_servers.size(); ++i) {
        const auto status = invalidIndexes.contains(i) ? DnsServerResult::StatusInvalid
                                                       : DnsServerResult::StatusSkipped;
        setResult(i, { .status = status });
    }

    return false;
}

void DnsServersTest::processAnswer(int index, const DnsAnswer &answer)
{
    // The late answer after the timeout
    if (m_results.at(index).status != DnsServerResult::StatusNone)
        return;

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

void DnsServersTest::processTimeout()
{
    const int msecs = int(m_elapsedTimer.elapsed());

    for (int i = 0; i < m_results.size(); ++i) {
        if (m_results.at(i).status == DnsServerResult::StatusNone) {
            setResult(i, { .status = DnsServerResult::StatusTimeout, .msecs = msecs });
        }
    }
}
