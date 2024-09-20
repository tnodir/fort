#include "formatutil.h"

#include <QRegularExpression>
#include <QtMath>

namespace {

constexpr int FORMAT_POWER_VALUES_SIZE = 7;

bool isBase1000(FormatUtil::SizeFormat format)
{
    return (format & FormatUtil::SizeBase1000) != 0;
}

bool isBits(FormatUtil::SizeFormat format)
{
    return (format & FormatUtil::SizeBits) != 0;
}

}

int FormatUtil::getPower(qint64 value, SizeFormat format)
{
    if (value == 0) {
        return 0;
    }

    if (isBase1000(format)) {
        return int(std::log10(qAbs(value)) / 3);
    }

    // Compute log2(value) / 10
    return int((63 - qCountLeadingZeroBits(quint64(qAbs(value)))) / 10);
}

QString FormatUtil::formatSize(qint64 value, int power, int precision, SizeFormat format)
{
    // We don't support sizes in units larger than exbibytes because
    // the number of bytes would not fit into qint64.
    Q_ASSERT(power >= 0 && power < FORMAT_POWER_VALUES_SIZE);

    if (power == 0) {
        return QLocale().toString(value);
    }

    const qreal base = isBase1000(format) ? 1000 : 1024;
    const qreal powerValue = qPow(base, power);

    const qreal result = value / powerValue;

    if (precision == -1) {
        precision = qFuzzyCompare(result, qRound(result)) ? 0 : 1;
    }

    Q_ASSERT(precision >= 0);

    return QLocale().toString(result, 'f', precision);
}

QString FormatUtil::formatPowerUnit(int power, SizeFormat format)
{
    const QString byteUnit(isBits(format) ? 'b' : 'B');
    if (power == 0) {
        return byteUnit;
    }

    static const char units[] = { 'K', 'M', 'G', 'T', 'P', 'E' };

    const QString unit(units[power - 1]);
    const QString unitSuffix = isBase1000(format) ? QLatin1String("i") : QString();

    return unit + unitSuffix + byteUnit;
}

QString FormatUtil::formatDataSize(qint64 bytes, int precision, SizeFormat format)
{
    const int power = getPower(bytes, format);
    const auto sizeStr = formatSize(bytes, power, precision, format);
    const auto unitStr = formatPowerUnit(power, format);

    return sizeStr + ' ' + unitStr;
}

QString FormatUtil::formatSpeed(qint64 bitsPerSecond, SizeFormat format)
{
    if (!isBits(format)) {
        bitsPerSecond /= 8;
    }

    const auto text = formatDataSize(bitsPerSecond, /*precision=*/-1, format);

    return text + "/s";
}

QStringList FormatUtil::graphUnitNames()
{
    static const QStringList list = { "b/s", "B/s", "ib/s", "iB/s" };

    return list;
}

FormatUtil::SizeFormat FormatUtil::graphUnitFormat(int index)
{
    static const SizeFormat list[] = { SpeedTraditionalFormat, SizeTraditionalFormat,
        SpeedIecFormat, SizeIecFormat };

    if (index < 0 || index >= std::size(list)) {
        index = 0;
    }

    return list[index];
}
