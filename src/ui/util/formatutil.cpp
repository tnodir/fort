#include "formatutil.h"

#include <QtMath>

namespace {

inline constexpr int FORMAT_POWER_VALUES_SIZE = 7;

bool isBase1000(FormatUtil::SizeFormat format)
{
    return (format & FormatUtil::SizeBase1000) != 0;
}

bool isBits(FormatUtil::SizeFormat format)
{
    return (format & FormatUtil::SizeBits) != 0;
}

qreal powerValue(int power, FormatUtil::SizeFormat format)
{
    const qreal base = isBase1000(format) ? 1000 : 1024;

    return qPow(base, power);
}

qint64 speedSize(qint64 bitsPerSecond, FormatUtil::SizeFormat format)
{
    return isBits(format) ? bitsPerSecond : bitsPerSecond / 8;
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

    const qreal result = value / powerValue(power, format);

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

int FormatUtil::getSpeedPower(qint64 bitsPerSecond, SizeFormat format)
{
    return getPower(speedSize(bitsPerSecond, format), format);
}

qreal FormatUtil::speedInUnit(qreal bitsPerSecond, int power, SizeFormat format)
{
    const qreal unitBits = isBits(format) ? 1 : 8;

    return bitsPerSecond / unitBits / powerValue(power, format);
}

QString FormatUtil::formatSpeedValue(
        qreal bitsPerSecond, int power, int precision, SizeFormat format)
{
    const qreal factor = qPow(10, precision);
    const qreal value = std::round(speedInUnit(bitsPerSecond, power, format) * factor) / factor;

    return QLocale().toString(value, 'f', QLocale::FloatingPointShortest);
}

QString FormatUtil::formatSpeedUnit(int power, SizeFormat format)
{
    return formatPowerUnit(power, format) + "/s";
}

QString FormatUtil::formatSpeed(qint64 bitsPerSecond, SizeFormat format)
{
    const qint64 value = speedSize(bitsPerSecond, format);
    const int power = getPower(value, format);

    return formatSize(value, power, /*precision=*/-1, format) + ' '
            + formatSpeedUnit(power, format);
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
