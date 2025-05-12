#include "trafunittype.h"

#include <util/dateutil.h>
#include <util/formatutil.h>

QString TrafUnitType::formatTrafUnit(qint64 bytes) const
{
    if (bytes == 0) {
        return "0";
    }

    const auto unit = this->unit();

    const int trafPrec = (unit == TrafUnitType::UnitBytes) ? 0 : 2;

    if (unit == TrafUnitType::UnitAdaptive) {
        return FormatUtil::formatDataSize(bytes, trafPrec);
    }

    const int power = unit - 1;

    return FormatUtil::formatSize(bytes, power, trafPrec);
}

QString TrafUnitType::formatTrafTime(qint32 trafTime) const
{
    const qint64 unixTime = DateUtil::toUnixTime(trafTime);

    switch (type()) {
    case TrafUnitType::TrafHourly:
        return DateUtil::formatHour(unixTime);
    case TrafUnitType::TrafDaily:
        return DateUtil::formatDay(unixTime);
    case TrafUnitType::TrafMonthly:
        return DateUtil::formatMonth(unixTime);
    case TrafUnitType::TrafTotal:
        return DateUtil::formatHour(unixTime);
    }

    return {};
}
