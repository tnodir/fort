#include "controlcommanddns.h"

#include <QPointer>

#include <dns/dnsserverstest.h>
#include <dns/dnsutil.h>

#include <conf/inioptions.h>
#include <control/controlworker.h>
#include <dns/dnsmanager.h>
#include <fortglobal.h>

#include "controlcommandmanager.h"
#include "controlcommandutil.h"

using namespace Fort;

namespace {

// Of each of the 3 waits: over DnsServersTest's timeout, a client's one included
inline constexpr int testResultWaitMsecs = 2 * 1000;

QString serverResultText(const DnsServerResult &result)
{
    switch (result.status) {
    case DnsServerResult::StatusAnswered:
        return DnsUtil::rcodeText(result.rcode) + QString(" %1 ms").arg(result.msecs);
    case DnsServerResult::StatusFailed:
        return "error: " + result.error;
    case DnsServerResult::StatusTimeout:
        return "timeout";
    case DnsServerResult::StatusLoop:
        return "loop";
    case DnsServerResult::StatusInvalid:
        return "invalid server";
    default:
        return {};
    }
}

void sendTestResult(ControlWorker *w, quint32 requestId, const DnsServersTest *test)
{
    ProcessCommandResult r;
    r.ok = true;

    const QString text = test->resultLines(&serverResultText, "adapters").join('\n');

    if (test->isAllNoError()) {
        r.errorMessage = text;
    } else {
        ControlCommandUtil::setErrorResult(r, text);
    }

    ControlCommandManager::sendResult(w, requestId, r);
}

bool testServers(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto dnsManager = Fort::dnsManager();
    if (!dnsManager) {
        r.errorMessage = "The servers are tested only with the service";
        return false;
    }

    const QPointer<ControlWorker> w = p.worker;
    const quint32 requestId = p.requestId;

    DnsServersTest *test = dnsManager->createServersTest(ini().dnsServers(), dnsManager);

    QObject::connect(test, &DnsServersTest::finished, test, [=] {
        if (w) {
            sendTestResult(w, requestId, test);
        }
        test->deleteLater();
    });

    r.isResultDeferred = true;

    test->start();

    return true;
}

}

bool ControlCommandDns::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    if (p.args.value(0).toString() != "test") {
        r.errorMessage = "Usage: dns test";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, /*action=*/0))
        return false;

    return testServers(p, r);
}

int ControlCommandDns::resultWaitMsecs(const QVariantList & /*args*/) const
{
    return testResultWaitMsecs;
}
