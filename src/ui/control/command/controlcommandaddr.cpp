#include "controlcommandaddr.h"

#include <functional>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortglobal.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

enum AddrAction : qint8 {
    AddrActionInvalid = -1,
    AddrActionSetText = 0,
    AddrActionGetText,
    AddrActionSetZones,
    AddrActionGetZones,
};

QStringList addrActionNames()
{
    // Sync with enum AddrAction
    return { "set-text", "get-text", "set-zones", "get-zones" };
}

QStringList addressListNames()
{
    // Sync with enum FirewallConf::AddressList, from AddressListLan
    return { "local-net", "allow-inet", "block-inet" };
}

AddrAction addrActionByText(const QString &commandText)
{
    return AddrAction(addrActionNames().indexOf(commandText));
}

int addrActionArgsCount(AddrAction addrAction)
{
    // The text actions have the <file-path> argument
    return (addrAction <= AddrActionGetText) ? 3 : 2;
}

AddrAction clientAddrAction(const QVariantList &args)
{
    const AddrAction addrAction = addrActionByText(args.value(0).toString());

    // The running instance reports the invalid arguments
    return (args.size() >= addrActionArgsCount(addrAction)) ? addrAction : AddrActionInvalid;
}

// Save the conf's copy changed by the function
bool saveConfCopy(
        ProcessCommandResult &r, const std::function<void(FirewallConf &conf)> &changeConf)
{
    auto confManager = Fort::confManager();

    FirewallConf conf;
    conf.copy(confManager->conf());

    changeConf(conf);
    conf.setOptEdited();

    if (!confManager->save(conf, ini())) {
        r.errorMessage = confManager->errorMessage();
        return false;
    }

    return true;
}

bool setAddressListText(
        ProcessCommandResult &r, FirewallConf::AddressList list, const QString &text)
{
    return saveConfCopy(r, [&](FirewallConf &conf) { conf.setAddressListText(list, text); });
}

bool setAddressListZones(
        ProcessCommandResult &r, FirewallConf::AddressList list, const QString &zonesText)
{
    QVariant zoneIds;
    if (!ControlCommandUtil::parseIds(zonesText, zoneIds)) {
        r.errorMessage = "Invalid zone ids: " + zonesText;
        return false;
    }

    const quint32 zonesMask = ControlCommandUtil::idsMask(zoneIds);

    return saveConfCopy(r, [&](FirewallConf &conf) { conf.setAddressListZones(list, zonesMask); });
}

bool processCommandAddrAction(
        ProcessCommandResult &r, const QVariantList &args, AddrAction addrAction)
{
    const auto list = FirewallConf::AddressList(
            FirewallConf::AddressListLan + addressListNames().indexOf(args.value(1).toString()));

    const auto &conf = confManager()->conf();

    switch (addrAction) {
    case AddrActionSetText: {
        return setAddressListText(r, list, args.value(2).toString());
    }
    case AddrActionGetText: {
        // The client writes the text to the file
        r.errorMessage = conf.addressListText(list);
        return true;
    }
    case AddrActionSetZones: {
        return setAddressListZones(r, list, args.value(2).toString());
    }
    default: // Get Zones
        r.errorMessage = ControlCommandUtil::idsText(
                ControlCommandUtil::maskIds(conf.addressListZones(list)));
        return true;
    }
}

}

bool ControlCommandAddr::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    const AddrAction addrAction = addrActionByText(p.args.value(0).toString());

    const bool isValidAction = (addrAction != AddrActionInvalid
            && addressListNames().contains(p.args.value(1).toString()));
    if (!isValidAction || p.args.size() < addrActionArgsCount(addrAction)) {
        r.errorMessage = "Usage: addr set-text|get-text <list> <file-path>"
                         " | addr set-zones <list> [<zone-id>,...] | addr get-zones <list>;"
                         " <list>: local-net|allow-inet|block-inet";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, addrAction))
        return false;

    return processCommandAddrAction(r, p.args, addrAction);
}

ControlCommandBase::ClientFileArg ControlCommandAddr::clientFileArg(const QVariantList &args) const
{
    switch (clientAddrAction(args)) {
    case AddrActionSetText:
        return ClientFileRead;
    case AddrActionGetText:
        return ClientFileWrite;
    default:
        return ClientFileNone;
    }
}
