#include "axistickerspeed.h"

#include <QtMath>

namespace {

inline constexpr int tickCount = 5;
inline constexpr int tickStepBase = 2;

}

void AxisTickerSpeed::generate(double rangeUpper, AxisTicks &axisTicks) const
{
    axisTicks = {};

    if (rangeUpper <= 0)
        return;

    const double tickStep = getTickStep(rangeUpper);
    const double subTickStep = tickStep / 2; // one sub tick between ticks
    const int lastStep = qFloor(rangeUpper / tickStep);

    for (int step = 0; step <= lastStep; ++step) {
        const double tick = step * tickStep;

        axisTicks.ticks.append(tick);
        axisTicks.labels.append(getTickLabel(tick));

        const double subTick = tick + subTickStep;
        if (subTick <= rangeUpper) {
            axisTicks.subTicks.append(subTick);
        }
    }
}

double AxisTickerSpeed::getTickStep(double rangeSize)
{
    const double exactStep = rangeSize / tickCount;

    return qPow(tickStepBase, qFloor(qLn(exactStep) / qLn(tickStepBase) + 0.5));
}

QString AxisTickerSpeed::getTickLabel(double tick) const
{
    return FormatUtil::formatSpeed(qint64(tick), unitFormat());
}
