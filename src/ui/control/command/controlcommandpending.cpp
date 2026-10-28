#include "controlcommandpending.h"

#include <fortglobal.h>
#include <model/apppendingconnsearchmodel.h>
#include <model/pendingconnsearchmodel.h>
#include <stat/pendingconnmanager.h>

#include "controlcommandconn.h"
#include "controlcommandutil.h"

using namespace Fort;

namespace {

enum PendingAction : qint8 {
    PendingActionInvalid = -1,
    PendingActionList = 0,
    PendingActionAllow,
    PendingActionBlock,
    PendingActionContinue,
    PendingActionAllowProc,
};

QStringList pendingActionNames()
{
    // Sync with enum PendingAction
    return { "list", "allow", "block", "continue", "allow-proc" };
}

PendingAction pendingActionByText(const QString &commandText)
{
    return PendingAction(pendingActionNames().indexOf(commandText));
}

int pendingActionArgsCount(PendingAction pendingAction)
{
    // The list action's <app-path> is optional
    return (pendingAction == PendingActionList) ? 1 : 2;
}

QStringList decisionNames()
{
    // Sync with enum PendingConnManager::ConnDecision
    return { "block", "allow", "continue" };
}

bool listPendingConns(ProcessCommandResult &r, const QVariantList &args)
{
    const auto model =
            ControlCommandConn::createConnModel<PendingConnSearchModel, AppPendingConnSearchModel>(
                    args.value(1).toString().trimmed());

    QVector<ConnRow> connRows;
    model->loadConnRows(/*connIdFrom=*/0, connRows);

    r.errorMessage = ControlCommandConn::connsText(connRows);

    return true;
}

// The pending connection of the <conn-id> argument, its id is 0 when not found
ConnRow pendingConnRow(const QVariantList &args)
{
    const qint64 connId = args.value(1).toLongLong();
    if (connId <= 0)
        return {};

    PendingConnSearchModel model;

    QVector<ConnRow> connRows;
    model.loadConnRows(connId - 1, connRows, /*count=*/1);

    const ConnRow connRow = connRows.value(0);

    return (connRow.connId == connId) ? connRow : ConnRow();
}

bool checkPendingConnFound(ProcessCommandResult &r, const ConnRow &connRow)
{
    if (connRow.connId == 0) {
        ControlCommandUtil::setErrorResult(r, "Pending connection not found");
        return false;
    }

    return true;
}

bool decidePendingConn(ProcessCommandResult &r, const QVariantList &args)
{
    const ConnRow connRow = pendingConnRow(args);
    if (!checkPendingConnFound(r, connRow))
        return true;

    const auto decision =
            PendingConnManager::ConnDecision(decisionNames().indexOf(args.value(0).toString()));

    // The inbound one's flow id is changed by its re-injection
    if (decision == PendingConnManager::ConnAllow && connRow.inbound) {
        r.errorMessage = "The inbound connection can't be allowed once";
        return false;
    }

    return pendingConnManager()->decideConn(connRow.connId, decision);
}

bool allowPendingConnProcess(ProcessCommandResult &r, const QVariantList &args)
{
    const ConnRow connRow = pendingConnRow(args);
    if (!checkPendingConnFound(r, connRow))
        return true;

    // 0 - till the process's exit
    bool ok = true;
    const quint32 seconds = (args.size() > 2) ? args[2].toUInt(&ok) : 0;
    if (!ok) {
        r.errorMessage = "Invalid <seconds>: " + args[2].toString();
        return false;
    }

    return pendingConnManager()->allowProcess(connRow.pid, seconds);
}

bool processPendingAction(
        ProcessCommandResult &r, const QVariantList &args, PendingAction pendingAction)
{
    switch (pendingAction) {
    case PendingActionList:
        return listPendingConns(r, args);
    case PendingActionAllowProc:
        return allowPendingConnProcess(r, args);
    default:
        return decidePendingConn(r, args);
    }
}

}

bool ControlCommandPending::processCommand(
        const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    const PendingAction pendingAction = pendingActionByText(p.args.value(0).toString());

    if (pendingAction == PendingActionInvalid
            || p.args.size() < pendingActionArgsCount(pendingAction)) {
        r.errorMessage = "Usage: pending list [<app-path>]"
                         " | pending allow|block|continue <conn-id>"
                         " | pending allow-proc <conn-id> [<seconds>]";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, pendingAction))
        return false;

    return processPendingAction(r, p.args, pendingAction);
}
