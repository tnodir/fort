#include "axistickerspeed.h"

#include <QLocale>
#include <QtMath>

namespace {

inline constexpr int tickCount = 5;
inline constexpr int tickStepBase = 2;

QString formatTickLabel(double value, int precision)
{
    const double factor = qPow(10, precision);
    const double roundedValue = std::round(value * factor) / factor;

    // Without the trailing zeros
    return QLocale().toString(roundedValue, 'f', QLocale::FloatingPointShortest);
}

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
    // The ticks are in bits
    const double unitBits = (m_unitFormat & FormatUtil::SizeBits) ? 1 : 8;
    const double base = (m_unitFormat & FormatUtil::SizeBase1000) ? 1000 : 1024;

    // The common unit of the labels is by the last tick
    const double lastTick = axisTicks.ticks.constLast() / unitBits;
    const int power = FormatUtil::getPower(qint64(lastTick), m_unitFormat);
    const double powerBits = qPow(base, power) * unitBits;

    const int precision = getLabelPrecision(tickStep / powerBits);

    for (const double tick : std::as_const(axisTicks.ticks)) {
        axisTicks.labels.append(formatTickLabel(tick / powerBits, precision));
    }

    axisTicks.unit = FormatUtil::formatPowerUnit(power, m_unitFormat) + "/s";
}

int AxisTickerSpeed::getLabelPrecision(double unitTickStep) const
{
    // The step is a power of 2 in bits: show its binary fraction exactly
    if (!(m_unitFormat & FormatUtil::SizeBase1000))
        return qMax(qCeil(-std::log2(unitTickStep)), 0);

    // Show 2 significant digits of the step
    return qMax(1 - qFloor(std::log10(unitTickStep)), 0);
}
