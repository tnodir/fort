#include "timeperiodscontroller.h"

#include <conf/conftimeperiodmanager.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>

using namespace Fort;

namespace {

void showErrorMessage(const QString &errorMessage)
{
    windowManager()->showErrorBox(
            errorMessage, TimePeriodsController::tr("Time Period Configuration Error"));
}

}

TimePeriodsController::TimePeriodsController(QObject *parent) : BaseController(parent) { }

bool TimePeriodsController::addOrUpdateTimePeriod(TimePeriod &period)
{
    if (!confTimePeriodManager()->addOrUpdateTimePeriod(period)) {
        showErrorMessage(tr("Cannot edit Time Period"));
        return false;
    }
    return true;
}

void TimePeriodsController::deleteTimePeriod(int periodId)
{
    if (!confTimePeriodManager()->deleteTimePeriod(periodId)) {
        showErrorMessage(tr("Cannot delete Time Period"));
    }
}

bool TimePeriodsController::updateTimePeriodName(int periodId, const QString &name)
{
    if (!confTimePeriodManager()->updateTimePeriodName(periodId, name)) {
        showErrorMessage(tr("Cannot update Time Period's name"));
        return false;
    }
    return true;
}
