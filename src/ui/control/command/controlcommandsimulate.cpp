#include "controlcommandsimulate.h"

#include <conf/confmanager.h>
#include <conf/filtersimconn.h>
#include <fortglobal.h>
#include <util/bitutil.h>
#include <util/net/dirrange.h>
#include <util/net/netutil.h>
#include <util/net/profilerange.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

QStringList resultNames()
{
    // Sync with enum DriverCommon::ConnFilterResult
    return { "allowed", "blocked", "ask", "ignored" };
}

QVariantMap defaultConnFields()
{
    // The Filter Simulator window's defaults
    return {
        { "port", QVariant::fromValue(quint16(443)) },
        { "proto", QString("tcp") },
        { "dir", QString("out") },
        { "localIp", QString() },
        { "localPort", QVariant::fromValue(quint16(0)) },
        { "loopback", false },
        { "profile", QString("public") },
    };
}

bool fillConnAddresses(ProcessCommandResult &r, FilterSimConn &simConn, const QString &remoteIpText,
        const QVariantMap &fields)
{
    if (!simConn.setRemoteIp(remoteIpText, fields["loopback"].toBool())) {
        r.errorMessage = "Invalid <remote-ip>: " + remoteIpText;
        return false;
    }

    const QString localIpText = fields["localIp"].toString();

    if (!simConn.setLocalIp(localIpText)) {
        r.errorMessage = ControlCommandUtil::invalidValueText("localIp", localIpText);
        return false;
    }

    return true;
}

// The field's id by its name, 0 - invalid
quint8 fieldIdByName(ProcessCommandResult &r, const QVariantMap &fields, const QString &key,
        quint8 (*idByName)(const QString &name))
{
    const QString text = fields[key].toString();
    const quint8 id = idByName(text);

    if (id == 0) {
        r.errorMessage = ControlCommandUtil::invalidValueText(key, text);
    }

    return id;
}

bool fillConnProto(ProcessCommandResult &r, FORT_CONF_META_CONN &conn, const QVariantMap &fields)
{
    const QString protoText = fields["proto"].toString();

    bool ok;
    conn.ip_proto = NetUtil::protocolNumber(protoText, ok);

    if (!ok) {
        r.errorMessage = ControlCommandUtil::invalidValueText("proto", protoText);
    }

    return ok;
}

bool fillConnFields(ProcessCommandResult &r, FORT_CONF_META_CONN &conn, const QVariantMap &fields)
{
    const quint8 directionId = fieldIdByName(r, fields, "dir", &DirRange::directionIdByName);
    const quint8 profileId = fieldIdByName(r, fields, "profile", &ProfileRange::profileIdByName);

    if (directionId == 0 || profileId == 0)
        return false;

    conn.inbound = (directionId == FORT_RULE_FILTER_DIRECTION_IN);
    // The profile's filter bit: 1 << profile_id
    conn.profile_id = BitUtil::bitScanForward(profileId);
    conn.local_port = fields["localPort"].toUInt();
    conn.remote_port = fields["port"].toUInt();

    return fillConnProto(r, conn, fields);
}

bool fillSimConn(ProcessCommandResult &r, FilterSimConn &simConn, const QVariantList &args)
{
    QVariantMap fields = defaultConnFields();

    if (!ControlCommandUtil::parseFieldArgs(args, 2, fields, r.errorMessage))
        return false;

    simConn.appPath = args.value(0).toString().trimmed();

    return fillConnAddresses(r, simConn, args.value(1).toString(), fields)
            && fillConnFields(r, simConn.conn, fields);
}

QVariantMap simConnFields(const FilterSimConn &simConn)
{
    const FORT_CONF_META_CONN &conn = simConn.conn;
    const FORT_APP_DATA &appData = conn.app.data;

    return {
        { "result", resultNames().value(simConn.result) },
        { "reason", int(conn.reason) },
        { "terminated", bool(conn.act.terminated) },
        { "rule", int(conn.rule_id) },
        { "zone", int(conn.act.zone_id) },
        { "app", appData.flags.found ? qint64(appData.app_id) : 0 },
    };
}

bool simulateConn(ProcessCommandResult &r, const QVariantList &args)
{
    FilterSimConn simConn;

    if (!fillSimConn(r, simConn, args))
        return false;

    if (!confManager()->simulateConn(simConn)) {
        r.errorMessage = "Cannot simulate the connection";
        return false;
    }

    r.errorMessage = ControlCommandUtil::fieldsToText(simConnFields(simConn));

    return true;
}

}

bool ControlCommandSimulate::processCommand(
        const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    if (p.args.size() < 2) {
        r.errorMessage = "Usage: simulate <app-path> <remote-ip> [<key>=<value>...];"
                         " keys: port, proto=tcp|udp|icmp|<number>, dir=out|in, localIp,"
                         " localPort, loopback=0|1, profile=public|private|domain";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, /*action=*/0))
        return false;

    return simulateConn(r, p.args);
}
