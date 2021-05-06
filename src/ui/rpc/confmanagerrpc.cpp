#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../conf/firewallconf.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../rpc/rpcmanager.h"

ConfManagerRpc::ConfManagerRpc(const QString &filePath, FortManager *fortManager, QObject *parent) :
    ConfManager(filePath, fortManager, parent, SqliteDb::OpenDefaultReadOnly)
{
}

RpcManager *ConfManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void ConfManagerRpc::onConfChanged(const QVariant &confVar)
{
    settings()->clearCache();

    FirewallConf *newConf = createConf();
    newConf->fromVariant(confVar, true);

    if (newConf->edited()) {
        // Reload from storage
        setConf(newConf);
        load();
    } else {
        // Apply only flags
        applySavedConf(newConf);
        delete newConf;
    }

    if (!saving()) {
        fortManager()->reloadOptionsWindow(tr("Settings changed by someone else"));
    }
}

bool ConfManagerRpc::saveConf(FirewallConf &newConf)
{
    setSaving(true);
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_save, { newConf.toVariant() });
    const bool timeouted = !rpcManager()->waitResult();
    setSaving(false);

    if (timeouted) {
        showErrorMessage("Save Conf: Service isn't responding.");
        return false;
    }

    if (rpcManager()->resultCommand() != Control::Rpc_Result_Ok) {
        showErrorMessage("Save Conf: Service error.");
        return false;
    }

    // Already applied by onConfChanged() & applySavedConf()
    newConf.resetEdited();

    return true;
}
