#include "dnsmanagerrpc.h"

#include <QPointer>

#include <control/controlworker.h>
#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

namespace {

inline constexpr int testTimeoutMargin = 500; // msecs: for the service's results to come

class DnsServersTestRpc : public DnsServersTest
{
public:
    explicit DnsServersTestRpc(
            DnsManagerRpc *dnsManager, const QStringList &servers, QObject *parent) :
        DnsServersTest(servers, parent), m_dnsManager(dnsManager)
    {
        // The service's test times out first
        setTimeout(timeout() + testTimeoutMargin);
    }

    // The service may not send the results: on its restart or the connection's loss
    void start() override
    {
        startTimeout();

        m_dnsManager->startServersTest(this);
    }

private:
    DnsManagerRpc *m_dnsManager = nullptr;
};

bool processDnsManager_updateState(const ProcessCommandArgs &p)
{
    if (auto dnsManager = qobject_cast<DnsManagerRpc *>(Fort::dnsManager())) {
        dnsManager->updateState({
                .status = DnsForwarderStatus::Status(p.args.value(0).toInt()),
                .errorMessage = p.args.value(1).toString(),
        });
    }
    return true;
}

QVariant resultToVariant(const DnsServerResult &result)
{
    return QVariantList { result.status, int(result.rcode), result.msecs, result.error };
}

DnsServerResult resultFromVariant(const QVariant &v)
{
    const QVariantList list = v.toList();

    return {
        .status = DnsServerResult::Status(list.value(0).toInt()),
        .rcode = quint8(list.value(1).toInt()),
        .msecs = list.value(2).toInt(),
        .error = list.value(3).toString(),
    };
}

bool processDnsManager_testServers(const ProcessCommandArgs &p)
{
    const QPointer<ControlWorker> w = p.worker;
    const quint32 testId = p.args.value(0).toUInt();

    auto dnsManager = Fort::dnsManager();

    DnsServersTest *test =
            dnsManager->createServersTest(p.args.value(1).toStringList(), dnsManager);

    QObject::connect(test, &DnsServersTest::resultReady, test, [=](int index) {
        if (w) {
            w->sendCommand(Control::Rpc_DnsManager_serverTested,
                    { testId, index, resultToVariant(test->results().at(index)) });
        }
    });
    QObject::connect(test, &DnsServersTest::finished, test, &QObject::deleteLater);

    test->start();

    return true;
}

bool processDnsManager_serverTested(const ProcessCommandArgs &p)
{
    if (auto dnsManager = qobject_cast<DnsManagerRpc *>(Fort::dnsManager())) {
        dnsManager->onServerTested(p.args.value(0).toUInt(), p.args.value(1).toInt(),
                resultFromVariant(p.args.value(2)));
    }
    return true;
}

}

DnsManagerRpc::DnsManagerRpc(QObject *parent) : DnsManager(parent) { }

DnsServersTest *DnsManagerRpc::createServersTest(const QStringList &servers, QObject *parent)
{
    return new DnsServersTestRpc(this, servers, parent);
}

void DnsManagerRpc::startServersTest(DnsServersTest *test)
{
    const quint32 testId = ++m_lastTestId;

    m_tests.insert(testId, test);

    connect(test, &QObject::destroyed, this, [=, this] { m_tests.remove(testId); });

    rpcManager()->invokeOnServer(Control::Rpc_DnsManager_testServers, { testId, test->servers() });
}

void DnsManagerRpc::onServerTested(quint32 testId, int index, const DnsServerResult &result)
{
    DnsServersTest *test = m_tests.value(testId);
    if (!test)
        return;

    test->setResult(index, result);
}

QVariantList DnsManagerRpc::updateState_args()
{
    const DnsForwarderStatus &status = Fort::dnsManager()->status();

    return { int(status.status), status.errorMessage };
}

bool DnsManagerRpc::processInitClient(ControlWorker *w)
{
    return w->sendCommand(Control::Rpc_DnsManager_updateState, updateState_args());
}

bool DnsManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    switch (p.command) {
    case Control::Rpc_DnsManager_updateState: {
        return processDnsManager_updateState(p);
    }
    case Control::Rpc_DnsManager_testServers: {
        return processDnsManager_testServers(p);
    }
    case Control::Rpc_DnsManager_serverTested: {
        return processDnsManager_serverTested(p);
    }
    default:
        return false;
    }
}

void DnsManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    connect(Fort::dnsManager(), &DnsManager::statusChanged, rpcManager, [=] {
        rpcManager->invokeOnClients(
                Control::Rpc_DnsManager_updateState, DnsManagerRpc::updateState_args());
    });
}
