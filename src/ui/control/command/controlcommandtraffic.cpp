#include "controlcommandtraffic.h"

#include <fortglobal.h>
#include <stat/statmanager.h>
#include <stat/statsql.h>
#include <util/fileutil.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

bool getAppTraffic(ProcessCommandResult &r, const QVariantList &args)
{
    auto statManager = Fort::statManager();

    const qint64 appId = statManager->getAppId(FileUtil::normalizePath(args.value(1).toString()));
    if (appId < 0) {
        ControlCommandUtil::setErrorResult(r, "App not found");
        return true;
    }

    // The total's time is any non-zero
    qint64 inBytes = 0, outBytes = 0;
    statManager->getTraffic(
            StatSql::sqlSelectTrafAppTotal, /*trafTime=*/1, inBytes, outBytes, appId);

    r.errorMessage = ControlCommandUtil::fieldsToText({ { "in", inBytes }, { "out", outBytes } });

    return true;
}

}

bool ControlCommandTraffic::processCommand(
        const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    if (p.args.value(0).toString() != "get" || p.args.size() < 2) {
        r.errorMessage = "Usage: traffic get <app-path>";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, /*action=*/0))
        return false;

    return getAppTraffic(r, p.args);
}
