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

// By DnsForwarderStatus::Status
inline constexpr const char *statusNames[] = {
    "off",
    "started",
    "nrpt-error",
    "mixed-servers",
    "invalid-servers",
    "listen-error",
};

bool reportStatus(ProcessCommandResult &r)
{
    auto dnsManager = Fort::dnsManager();
    if (!dnsManager) {
        r.errorMessage = "The DNS Forwarder runs only with the service";
        return false;
    }

    const DnsForwarderStatus &status = dnsManager->status();

    r.commandResult = Control::CommandResult(Control::CommandResultBase + status.status);

    r.errorMessage = statusNames[status.status];

    if (!status.errorMessage.isEmpty()) {
        r.errorMessage += ": " + status.errorMessage;
    }

    return true;
}

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
    case DnsServerResult::StatusMixed:
        return "mixed protocols";
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
    const QString commandText = p.args.value(0).toString();

    const bool report = (commandText == "report");

    if (!report && commandText != "test") {
        r.errorMessage = "Usage: dns test|report";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, /*action=*/0))
        return false;

    return report ? reportStatus(r) : testServers(p, r);
}

int ControlCommandDns::resultWaitMsecs(const QVariantList &args) const
{
    return (args.value(0).toString() == "test") ? testResultWaitMsecs
                                                : ControlCommandBase::resultWaitMsecs(args);
}
