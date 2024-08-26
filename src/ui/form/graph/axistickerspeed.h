#ifndef AXISTICKERSPEED_H
#define AXISTICKERSPEED_H

#include <qcustomplot.h>

#include <util/formatutil.h>

class AxisTickerSpeed : public QCPAxisTicker
{
    Q_GADGET

public:
    explicit AxisTickerSpeed() = default;

    FormatUtil::SizeFormat unitFormat() const { return m_unitFormat; }
    void setUnitFormat(FormatUtil::SizeFormat v) { m_unitFormat = v; }

protected:
    double getTickStep(const QCPRange &range) override;
    QString getTickLabel(
            double tick, const QLocale &locale, QChar formatChar, int precision) override;

private:
    FormatUtil::SizeFormat m_unitFormat = FormatUtil::SpeedTraditionalFormat;
};

#endif // AXISTICKERSPEED_H
