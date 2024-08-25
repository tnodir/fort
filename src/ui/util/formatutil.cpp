#include "formatutil.h"

namespace {

static const qint64 g_baseSiPowerValues[] = {
    1,
    1024LL, // kilo
    1024LL * 1024LL, // mega
    1024LL * 1024LL * 1024LL, // giga
    1024LL * 1024LL * 1024LL * 1024LL, // tera
    1024LL * 1024LL * 1024LL * 1024LL * 1024LL, // peta
    1024LL * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL, // exa
};

static const qint64 g_baseIecPowerValues[] = {
    1,
    1000LL, // kibi
    1000LL * 1000LL, // mebi
    1000LL * 1000LL * 1000LL, // gibi
    1000LL * 1000LL * 1000LL * 1000LL, // tebi
    1000LL * 1000LL * 1000LL * 1000LL * 1000LL, // pebi
    1000LL * 1000LL * 1000LL * 1000LL * 1000LL * 1000LL, // exbi
};

constexpr int FORMAT_POWER_VALUES_SIZE = 7;

static_assert(
        std::size(g_baseSiPowerValues) == FORMAT_POWER_VALUES_SIZE, "powerValues size mismatch");

}

QString FormatUtil::formatSize(
        qint64 value, int power, int precision, QLocale::DataSizeFormats format)
{
    // We don't support sizes in units larger than exbibytes because
    // the number of bytes would not fit into qint64.
    Q_ASSERT(power < FORMAT_POWER_VALUES_SIZE && power >= 0);

    const qint64 *powerValues =
            (format & QLocale::DataSizeBase1000) ? g_baseIecPowerValues : g_baseSiPowerValues;
    const qint64 powerValue = powerValues[power];

    return QLocale().toString(qreal(value) / powerValue, 'f', precision);
}

QString FormatUtil::formatDataSize(qint64 bytes, int precision, QLocale::DataSizeFormats format)
{
    return QLocale().formattedDataSize(bytes, precision, format);
}

QString FormatUtil::formatSpeed(quint32 bitsPerSecond)
{
    const QString text = formatDataSize(bitsPerSecond, /*precision=*/0);

    return text + QObject::tr("/s");
}
