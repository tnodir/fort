#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <conf/filtersimconn.h>
#include <conf/firewallconf.h>
#include <control/controlworker.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <rpc/rpcmanager.h>
#include <util/net/netutil.h>
#include <util/variantutil.h>

using namespace Fort;

namespace {

inline bool processConfManager_confChanged(ConfManager *confManager, const ProcessCommandArgs &p)
{
    if (auto cm = qobject_cast<ConfManagerRpc *>(confManager)) {
        cm->onConfChanged(p.args.value(0));
    }
    return true;
}

bool processConfManager_saveVariant(
        ConfManager *confManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confManager->saveVariant(p.args.value(0));
}

bool processConfManager_exportMasterBackup(
        ConfManager *confManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confManager->exportMasterBackup(p.args.value(0).toString());
}

bool processConfManager_importMasterBackup(
        ConfManager *confManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confManager->importMasterBackup(p.args.value(0).toString());
}

bool processConfManager_checkPassword(
        ConfManager *confManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    const bool ok = confManager->checkPassword(p.args.value(0).toString());
    if (ok && !p.worker->isClientValidated()) {
        p.worker->setIsClientValidated(true);
    }
    return ok;
}

bool processConfManager_simulateConn(
        ConfManager *confManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    FilterSimConn simConn = ConfManagerRpc::varListToSimConn(p.args);

    if (!confManager->simulateConn(simConn))
        return false;

    r.args = ConfManagerRpc::simConnToVarList(simConn);

    return true;
}

using processConfManager_func = bool (*)(
        ConfManager *confManager, const ProcessCommandArgs &p, ProcessCommandResult &r);

static const processConfManager_func processConfManager_funcList[] = {
    &processConfManager_saveVariant, // Rpc_ConfManager_saveVariant,
    &processConfManager_exportMasterBackup, // Rpc_ConfManager_exportMasterBackup,
    &processConfManager_importMasterBackup, // Rpc_ConfManager_importMasterBackup,
    &processConfManager_checkPassword, // Rpc_ConfManager_checkPassword,
    &processConfManager_simulateConn, // Rpc_ConfManager_simulateConn,
};

inline bool processConfManagerRpcResult(
        ConfManager *confManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const processConfManager_func func =
            RpcManager::getProcessFunc(p.command, processConfManager_funcList,
                    Control::Rpc_ConfManager_saveVariant, Control::Rpc_ConfManager_simulateConn);

    return func ? func(confManager, p, r) : false;
}

}

ConfManagerRpc::ConfManagerRpc(const QString &filePath, QObject *parent) :
    ConfManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

bool ConfManagerRpc::saveConf(FirewallConf &conf, IniOptions &ini)
{
    const QVariant confVar = conf.toVariant(ini, /*onlyEdited=*/true);

    setSaving(true);
    const bool ok = saveVariant(confVar);
    setSaving(false);

    if (!ok)
        return false;

    // Already applied by onConfChanged() & applySavedConf()
    conf.resetEdited();

    return true;
}

bool ConfManagerRpc::saveVariant(const QVariant &confVar)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_saveVariant, { confVar });
}

bool ConfManagerRpc::exportMasterBackup(const QString &path)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_exportMasterBackup, { path });
}

bool ConfManagerRpc::importMasterBackup(const QString &path)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_importMasterBackup, { path });
}

bool ConfManagerRpc::checkPassword(const QString &password)
{
    return rpcManager()->doOnServer(Control::Rpc_ConfManager_checkPassword, { password });
}

bool ConfManagerRpc::simulateConn(FilterSimConn &simConn)
{
    QVariantList resArgs;

    if (!rpcManager()->doOnServer(
                Control::Rpc_ConfManager_simulateConn, simConnToVarList(simConn), &resArgs))
        return false;

    simConn = varListToSimConn(resArgs);

    return true;
}

void ConfManagerRpc::onConfChanged(const QVariant &confVar)
{
    settings()->clearCache(); // FirewallConf::IniEdited is handled here

    const uint editedFlags = FirewallConf::editedFlagsFromVariant(confVar);

    if ((editedFlags & FirewallConf::OptEdited) != 0) {
        // Reload from storage
        load();
    } else {
        // Apply only flags
        conf().fromVariant(ini(), confVar, /*onlyEdited=*/true);

        applySavedConf(conf());
    }

    if (!saving()) {
        windowManager()->reloadOptionsWindow(tr("Settings changed by someone else"));
    }
}

QVariantList ConfManagerRpc::simConnToVarList(const FilterSimConn &simConn)
{
    const FORT_CONF_META_CONN &conn = simConn.conn;

    return { int(simConn.result), bool(conn.inbound), bool(conn.isIPv6), int(conn.profile_id),
        bool(conn.is_loopback), conn.ip_proto, conn.local_port, conn.remote_port,
        NetUtil::ip6ToArrayView(conn.local_ip.v6).toByteArray(),
        NetUtil::ip6ToArrayView(conn.remote_ip.v6).toByteArray(), conn.reason, conn.rule_id,
        conn.act.zone_id, conn.app_data.app_id, bool(conn.app_data.flags.found), simConn.appPath };
}

FilterSimConn ConfManagerRpc::varListToSimConn(const QVariantList &v)
{
    FilterSimConn simConn;
    simConn.result = DriverCommon::ConnFilterResult(v.value(0).toInt());

    FORT_CONF_META_CONN &conn = simConn.conn;
    conn.inbound = v.value(1).toBool();
    conn.isIPv6 = v.value(2).toBool();
    conn.profile_id = v.value(3).toUInt();
    conn.is_loopback = v.value(4).toBool();
    conn.ip_proto = v.value(5).toUInt();
    conn.local_port = v.value(6).toUInt();
    conn.remote_port = v.value(7).toUInt();
    conn.local_ip.v6 = NetUtil::arrayViewToIp6(v.value(8).toByteArray());
    conn.remote_ip.v6 = NetUtil::arrayViewToIp6(v.value(9).toByteArray());
    conn.reason = v.value(10).toUInt();
    conn.rule_id = v.value(11).toUInt();
    conn.act.zone_id = v.value(12).toUInt();
    conn.app_data.app_id = v.value(13).toUInt();
    conn.app_data.flags.found = v.value(14).toBool();

    simConn.appPath = v.value(15).toString();

    return simConn;
}

bool ConfManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto confManager = Fort::confManager();

    switch (p.command) {
    case Control::Rpc_ConfManager_confChanged: {
        return processConfManager_confChanged(confManager, p);
    }
    case Control::Rpc_ConfManager_imported: {
        emit confManager->imported();
        return true;
    }
    default: {
        r.ok = processConfManagerRpcResult(confManager, p, r);
        r.isSendResult = true;
        return true;
    }
    }
}

void ConfManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confManager = Fort::confManager();

    connect(confManager, &ConfManager::confChanged, rpcManager,
            [=](bool onlyFlags, uint editedFlags) {
                const QVariant confVar =
                        Fort::confManager()->toPatchVariant(Fort::ini(), onlyFlags, editedFlags);
                rpcManager->invokeOnClients(Control::Rpc_ConfManager_confChanged, { confVar });
            });
    connect(confManager, &ConfManager::imported, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfManager_imported); });
}
