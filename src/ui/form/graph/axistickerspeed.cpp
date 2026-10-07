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

        const double subTick = tick + subTickStep;
        if (subTick <= rangeUpper) {
            axisTicks.subTicks.append(subTick);
        }
    }

    setupLabels(tickStep, axisTicks);
}

double AxisTickerSpeed::getTickStep(double rangeSize)
{
    const double exactStep = rangeSize / tickCount;

    return qPow(tickStepBase, qFloor(qLn(exactStep) / qLn(tickStepBase) + 0.5));
}

void AxisTickerSpeed::setupLabels(double tickStep, AxisTicks &axisTicks) const
{
    // The common unit of the labels is by the last tick
    const qint64 lastTick = qint64(axisTicks.ticks.constLast());
    const int power = FormatUtil::getSpeedPower(lastTick, m_unitFormat);

    const int precision = getLabelPrecision(FormatUtil::speedInUnit(tickStep, power, m_unitFormat));

    for (const double tick : std::as_const(axisTicks.ticks)) {
        axisTicks.labels.append(FormatUtil::formatSpeedValue(tick, power, precision, m_unitFormat));
    }

    axisTicks.unit = FormatUtil::formatSpeedUnit(power, m_unitFormat);
}

int AxisTickerSpeed::getLabelPrecision(double unitTickStep) const
{
    // The step is a power of 2 in bits: show its binary fraction exactly
    if (!(m_unitFormat & FormatUtil::SizeBase1000))
        return qMax(qCeil(-std::log2(unitTickStep)), 0);

    // Show 2 significant digits of the step
    return qMax(1 - qFloor(std::log10(unitTickStep)), 0);
}
