#include "formatutil.h"

QString FormatUtil::formatDataSize(qint64 bytes, int precision, QLocale::DataSizeFormats format)
{
    return QLocale().formattedDataSize(bytes, precision, format);
}

QString FormatUtil::formatSpeed(quint32 bitsPerSecond)
{
    const QString text = formatDataSize(bitsPerSecond, /*precision=*/0);

    return text + QObject::tr("/s");
}
