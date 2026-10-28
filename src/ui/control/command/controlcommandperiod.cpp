#include "controlcommandperiod.h"

#include <conf/conftimeperiodmanager.h>
#include <conf/timeperiod.h>
#include <fortglobal.h>
#include <util/conf/confutil.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

enum PeriodAction : qint8 {
    PeriodActionInvalid = -1,
    PeriodActionAdd = 0,
    PeriodActionDel,
};

QStringList periodActionNames()
{
    // Sync with enum PeriodAction
    return { "add", "del" };
}

PeriodAction periodActionByText(const QString &commandText)
{
    return PeriodAction(periodActionNames().indexOf(commandText));
}

int periodActionArgsCount(PeriodAction periodAction)
{
    // The add action has the <intervals> argument
    return (periodAction == PeriodActionAdd) ? 3 : 2;
}

bool addTimePeriod(ProcessCommandResult &r, const QVariantList &args)
{
    TimePeriod period;
    period.name = args.value(1).toString();

    if (period.name.isEmpty()) {
        r.errorMessage = "<period-name> is empty";
        return false;
    }

    if (!TimePeriod::parseIntervals(args.value(2).toString(), period.intervals)) {
        r.errorMessage = "Invalid intervals: " + args.value(2).toString();
        return false;
    }

    QVariantMap fields = { { "enabled", period.enabled }, { "notes", period.notes } };

    if (!ControlCommandUtil::parseFieldArgs(args, 3, fields, r.errorMessage))
        return false;

    period.enabled = fields["enabled"].toBool();
    period.notes = fields["notes"].toString();

    if (!confTimePeriodManager()->addOrUpdateTimePeriod(period))
        return false;

    r.errorMessage = QString::number(period.periodId);

    return true;
}

quint8 argPeriodId(const QVariantList &args)
{
    const uint periodId = args.value(1).toUInt();

    return (periodId <= uint(ConfUtil::timePeriodMaxCount())) ? periodId : 0;
}

bool deleteTimePeriod(ProcessCommandResult &r, const QVariantList &args)
{
    auto confTimePeriodManager = Fort::confTimePeriodManager();

    const quint8 periodId = argPeriodId(args);

    if (periodId == 0 || confTimePeriodManager->timePeriodNameById(periodId).isEmpty()) {
        ControlCommandUtil::setErrorResult(r, "Time Period not found");
        return true;
    }

    return confTimePeriodManager->deleteTimePeriod(periodId);
}

}

bool ControlCommandPeriod::processCommand(
        const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    const PeriodAction periodAction = periodActionByText(p.args.value(0).toString());

    if (periodAction == PeriodActionInvalid
            || p.args.size() < periodActionArgsCount(periodAction)) {
        r.errorMessage =
                "Usage: period add <period-name> <intervals> [<key>=<value>...]"
                " | period del <period-id>;"
                " <intervals>: <days>/<hh:mm>-<hh:mm>,... e.g. 1-5/09:00-18:00,6,7/10:00-12:00";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, periodAction))
        return false;

    return (periodAction == PeriodActionAdd) ? addTimePeriod(r, p.args)
                                             : deleteTimePeriod(r, p.args);
}
