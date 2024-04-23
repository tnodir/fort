#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include <conf/firewallconf.h>
#include <control/controlworker.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <rpc/rpcmanager.h>
#include <task/taskmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/variantutil.h>

namespace {

inline bool processConfManager_confChanged(ConfManager *confManager, const ProcessCommandArgs &p)
{
    if (auto cm = qobject_cast<ConfManagerRpc *>(confManager)) {
        cm->onConfChanged(p.args.value(0));
    }
    return true;
}

bool processConfManager_saveVariant(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->saveVariant(p.args.value(0));
}

bool processConfManager_exportMasterBackup(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->exportMasterBackup(p.args.value(0).toString());
}

bool processConfManager_importMasterBackup(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confManager->importMasterBackup(p.args.value(0).toString());
}

bool processConfManager_checkPassword(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    const bool ok = confManager->checkPassword(p.args.value(0).toString());
    if (ok && !p.worker->isClientValidated()) {
        p.worker->setIsClientValidated(true);
    }
    return ok;
}

using processConfManager_func = bool (*)(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs);

static processConfManager_func processConfManager_funcList[] = {
    &processConfManager_saveVariant, // Rpc_ConfManager_saveVariant,
    &processConfManager_exportMasterBackup, // Rpc_ConfManager_exportMasterBackup,
    &processConfManager_importMasterBackup, // Rpc_ConfManager_importMasterBackup,
    &processConfManager_checkPassword, // Rpc_ConfManager_checkPassword,
};

inline bool processConfManagerRpcResult(
        ConfManager *confManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const processConfManager_func func =
            RpcManager::getProcessFunc(p.command, processConfManager_funcList,
                    Control::Rpc_ConfManager_saveVariant, Control::Rpc_ConfManager_checkPassword);

    return func ? func(confManager, p, resArgs) : false;
}

}

ConfManagerRpc::ConfManagerRpc(const QString &filePath, QObject *parent) :
    ConfManager(filePath, parent, SqliteDb::OpenDefaultReadOnly)
{
}

bool ConfManagerRpc::exportMasterBackup(const QString &path)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_exportMasterBackup, { path });
}

bool ConfManagerRpc::importMasterBackup(const QString &path)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_importMasterBackup, { path });
}

bool ConfManagerRpc::checkPassword(const QString &password)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_checkPassword, { password });
}

bool ConfManagerRpc::saveConf(FirewallConf &newConf)
{
    Q_ASSERT(&newConf == conf() || &newConf == confToEdit()); // else newConf.deleteLater()

    newConf.prepareToSave();

    const QVariant confVar = newConf.toVariant(/*onlyEdited=*/true);

    setSaving(true);
    const bool ok =
            IoC<RpcManager>()->doOnServer(Control::Rpc_ConfManager_saveVariant, { confVar });
    setSaving(false);

    if (!ok)
        return false;

    // Already applied by onConfChanged() & applySavedConf()
    newConf.resetEdited();

    if (&newConf == confToEdit()) {
        setConfToEdit(nullptr);
    }

    return true;
}

void ConfManagerRpc::onConfChanged(const QVariant &confVar)
{
    IoC<FortSettings>()->clearCache(); // FirewallConf::IniEdited is handled here

    const uint editedFlags = FirewallConf::editedFlagsFromVariant(confVar);

    if ((editedFlags & FirewallConf::OptEdited) != 0) {
        // Reload from storage
        setConf(createConf());
        loadConf(*conf());
    } else {
        // Apply only flags
        conf()->fromVariant(confVar, /*onlyEdited=*/true);
    }

    if ((editedFlags & FirewallConf::TaskEdited) != 0) {
        IoC<TaskManager>()->loadSettings();
    }

    applySavedConf(conf());

    if (!saving()) {
        IoC<WindowManager>()->reloadOptionsWindow(tr("Settings changed by someone else"));
    }
}

bool ConfManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult)
{
    auto confManager = IoC<ConfManager>();

    switch (p.command) {
    case Control::Rpc_ConfManager_confChanged: {
        return processConfManager_confChanged(confManager, p);
    }
    default: {
        ok = processConfManagerRpcResult(confManager, p, resArgs);
        isSendResult = true;
        return true;
    }
    }
}

void ConfManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confManager = IoC<ConfManager>();

    connect(confManager, &ConfManager::confChanged, rpcManager, [=](bool onlyFlags) {
        const QVariant confVar = IoC<ConfManager>()->toPatchVariant(onlyFlags);
        rpcManager->invokeOnClients(Control::Rpc_ConfManager_confChanged, { confVar });
    });
}
