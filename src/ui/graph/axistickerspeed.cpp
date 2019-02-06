#include "axistickerspeed.h"

#include "../util/net/netutil.h"

AxisTickerSpeed::AxisTickerSpeed()
{
}

double AxisTickerSpeed::getTickStep(const QCPRange &range)
{
    const double exactStep = range.size() / tickCount();
    const int tickStep = 2;

    return qPow(tickStep, int(qLn(exactStep) / qLn(tickStep) + 0.5));
}

QString AxisTickerSpeed::getTickLabel(double tick, const QLocale &locale,
                                      QChar formatChar, int precision)

{
    Q_UNUSED(locale)
    Q_UNUSED(formatChar)
    Q_UNUSED(precision)

    return NetUtil::formatSpeed(tick);
}
