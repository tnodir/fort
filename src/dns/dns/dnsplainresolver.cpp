#include "dnsplainresolver.h"

#include <QLoggingCategory>

#include <memory>

#include <ares.h>

#include "dnsutil.h"

struct DnsResolveContext
{
    quint16 qid = 0; // set by c-ares for the sent query only
    DnsPlainResolver *resolver = nullptr;
    DnsRequest request;
};

namespace {

const QLoggingCategory LC("dns.plainResolver");

}

DnsPlainResolver::DnsPlainResolver(QObject *parent) : DnsResolver(parent)
{
    ares_library_init(ARES_LIB_INIT_ALL);
}

DnsPlainResolver::~DnsPlainResolver()
{
    close();

    ares_library_cleanup();
}

bool DnsPlainResolver::open(const QString &servers)
{
    close();

    if (initChannel() && setServers(servers))
        return true;

    close();
    return false;
}

void DnsPlainResolver::close()
{
    if (!m_channel)
        return;

    // Calls back the pending queries with ARES_EDESTRUCTION
    ares_destroy(m_channel);
    m_channel = nullptr;

    m_pendingQueries.clear();
}

bool DnsPlainResolver::isOwnQuery(const DnsQuery &query) const
{
    const DnsResolveContext *context = m_pendingQueries.value(query.id());

    return context && DnsUtil::isSameQuestion(context->request.query.record(), query.record());
}

void DnsPlainResolver::resolve(const DnsRequest &request)
{
    auto context = new DnsResolveContext { .resolver = this, .request = request };

    // Calls back on errors too
    const ares_status_t status = ares_send_dnsrec(
            m_channel, request.query.record(),
            [](void *arg, ares_status_t status, size_t /*timeouts*/,
                    const ares_dns_record_t *record) { onResolved(arg, status, record); },
            context, &context->qid);

    // The sent query is called back on the c-ares thread, after the return
    if (status == ARES_SUCCESS) {
        m_pendingQueries.insert(context->qid, context);
    }
}

bool DnsPlainResolver::initChannel()
{
    ares_options options = {};
    options.evsys = ARES_EVSYS_DEFAULT;
    // Without the cache: the clients cache themselves, and a cached answer has no query id
    options.qcache_max_ttl = 0;

    const int status =
            ares_init_options(&m_channel, &options, ARES_OPT_EVENT_THREAD | ARES_OPT_QUERY_CACHE);
    if (status != ARES_SUCCESS) {
        qCWarning(LC) << "Channel init error:" << ares_strerror(status);
        m_channel = nullptr;
        return false;
    }

    return true;
}

bool DnsPlainResolver::setServers(const QString &servers)
{
    if (servers.isEmpty())
        return true;

    const int status = ares_set_servers_csv(m_channel, servers.toLatin1().constData());
    if (status != ARES_SUCCESS) {
        qCWarning(LC) << "Servers error:" << servers << ares_strerror(status);
        return false;
    }

    return true;
}

void DnsPlainResolver::removePendingQuery(const DnsResolveContext *context)
{
    // The query failed at once isn't pending
    if (m_pendingQueries.value(context->qid) == context) {
        m_pendingQueries.remove(context->qid);
    }
}

void DnsPlainResolver::onResolved(void *arg, int status, const ares_dns_record *record)
{
    const std::shared_ptr<DnsResolveContext> context(static_cast<DnsResolveContext *>(arg));

    if (status == ARES_EDESTRUCTION)
        return;

    const DnsRequest &request = context->request;

    DnsAnswer answer;
    if (record) {
        answer.setRecord(request.query, record, request.responseSizeMax());
    } else {
        answer.setRcode(request.query, ARES_RCODE_SERVFAIL);
    }

    DnsPlainResolver *resolver = context->resolver;

    // To the resolver's thread
    QMetaObject::invokeMethod(
            resolver,
            [=] {
                resolver->removePendingQuery(context.get());

                emit resolver->resolved(context->request, answer);
            },
            Qt::QueuedConnection);
}
