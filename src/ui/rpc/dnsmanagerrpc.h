#ifndef DNSMANAGERRPC_H
#define DNSMANAGERRPC_H

#include <QHash>

#include <control/control_types.h>
#include <dns/dnsmanager.h>

class ControlWorker;
class RpcManager;

class DnsManagerRpc : public DnsManager
{
    Q_OBJECT

public:
    explicit DnsManagerRpc(QObject *parent = nullptr);

    void setUp() override { }
    void tearDown() override { }

    void updateState(const DnsForwarderStatus &status) { setStatus(status); }

    // The test asks the servers on the service
    DnsServersTest *createServersTest(const QStringList &servers, QObject *parent) override;

    void startServersTest(DnsServersTest *test);

    void onServerTested(quint32 testId, int index, const DnsServerResult &result);

    static QVariantList updateState_args();

    static bool processInitClient(ControlWorker *w);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

private:
    quint32 m_lastTestId = 0;

    QHash<quint32, DnsServersTest *> m_tests;
};

#endif // DNSMANAGERRPC_H
