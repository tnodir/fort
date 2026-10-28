#include "controlcommandconn.h"

#include <conf/confappmanager.h>
#include <fortglobal.h>
#include <model/appconnsearchmodel.h>
#include <util/net/dirrange.h>
#include <util/net/netformatutil.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

inline constexpr int connsLimitDefault = 100;

QString addressText(const ip_addr_t &ip, quint16 port, bool isIPv6)
{
    return NetFormatUtil::ipToAddressText(ip, isIPv6) + ':' + QString::number(port);
}

QVariantMap connFields(const ConnRow &connRow)
{
    return {
        { "blocked", connRow.blocked },
        { "dropped", connRow.dropped },
        { "alerted", connRow.alerted },
        { "terminated", connRow.terminated },
        { "inherited", connRow.inherited },
        { "dir", DirRange::directionName(connRow.inbound).toLower() },
        { "loopback", connRow.loopback },
        { "reason", int(connRow.reason) },
        { "proto", int(connRow.ipProto) },
        { "zone", int(connRow.zoneId) },
        { "rule", int(connRow.ruleId) },
        { "local", addressText(connRow.localIp, connRow.localPort, connRow.isIPv6) },
        { "remote", addressText(connRow.remoteIp, connRow.remotePort, connRow.isIPv6) },
        { "pid", connRow.pid },
        { "path", connRow.appPath },
        { "time", connRow.connTime.toSecsSinceEpoch() },
        { "id", connRow.connId },
    };
}

void fillConnsText(ProcessCommandResult &r, const QVector<ConnRow> &connRows)
{
    QStringList lines;

    for (const ConnRow &connRow : connRows) {
        lines.append(ControlCommandUtil::fieldsToText(connFields(connRow)));
    }

    r.errorMessage = lines.join('\n');
}

bool listConns(ProcessCommandResult &r, const QVariantList &args)
{
    // Without "after": the last connections
    QVariantMap fields = {
        { "after", QVariant::fromValue(qint64(-1)) },
        { "limit", connsLimitDefault },
    };

    const bool hasAppPath =
            args.size() > 1 && !ControlCommandUtil::isFieldArg(args[1].toString(), fields);

    if (!ControlCommandUtil::parseFieldArgs(args, hasAppPath ? 2 : 1, fields, r.errorMessage))
        return false;

    const int limit = fields["limit"].toInt();
    if (limit <= 0) {
        r.errorMessage = ControlCommandUtil::invalidValueText("limit", QString::number(limit));
        return false;
    }

    const auto model = ControlCommandConn::createConnModel<ConnSearchModel, AppConnSearchModel>(
            hasAppPath ? args[1].toString().trimmed() : QString());

    const qint64 after = fields["after"].toLongLong();
    const qint64 connIdFrom = (after >= 0) ? after : model->searchConnIdMin(limit) - 1;

    QVector<ConnRow> connRows;
    model->loadConnRows(connIdFrom, connRows, limit);

    fillConnsText(r, connRows);

    return true;
}

}

bool ControlCommandConn::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    if (p.args.value(0).toString() != "list") {
        r.errorMessage = "Usage: conn list [<app-path>] [after=<conn-id>] [limit=<count>]";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, /*action=*/0))
        return false;

    return listConns(r, p.args);
}

void ControlCommandConn::setModelApp(AppConnSearchModel &model, const QString &appPath)
{
    model.setApp(confAppManager()->appByPath(appPath));
}
