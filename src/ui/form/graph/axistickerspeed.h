#ifndef AXISTICKERSPEED_H
#define AXISTICKERSPEED_H

#include <qcustomplot.h>

class AxisTickerSpeed : public QCPAxisTicker
{
    Q_GADGET

public:
    explicit AxisTickerSpeed() = default;

protected:
    double getTickStep(const QCPRange &range) override;
    QString getTickLabel(
            double tick, const QLocale &locale, QChar formatChar, int precision) override;
};

#endif // AXISTICKERSPEED_H
