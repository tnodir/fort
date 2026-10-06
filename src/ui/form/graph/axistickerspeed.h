#ifndef AXISTICKERSPEED_H
#define AXISTICKERSPEED_H

#include <QStringList>
#include <QVector>

#include <util/formatutil.h>

struct AxisTicks
{
    QVector<double> ticks;
    QVector<double> subTicks;
    QStringList labels;
};

class AxisTickerSpeed
{
public:
    explicit AxisTickerSpeed() = default;

    FormatUtil::SizeFormat unitFormat() const { return m_unitFormat; }
    void setUnitFormat(FormatUtil::SizeFormat v) { m_unitFormat = v; }

    // Generates the ticks of the [0, rangeUpper] range
    void generate(double rangeUpper, AxisTicks &axisTicks) const;

private:
    static double getTickStep(double rangeSize);

    QString getTickLabel(double tick) const;

private:
    FormatUtil::SizeFormat m_unitFormat = FormatUtil::SpeedTraditionalFormat;
};

#endif // AXISTICKERSPEED_H
