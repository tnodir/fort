#include "formatutil.h"

#include <QLocale>

QString FormatUtil::formatDataSize(qint64 bytes, int precision)
{
    return QLocale().formattedDataSize(bytes, precision, QLocale::DataSizeTraditionalFormat);
}

QString FormatUtil::formatSpeed(quint32 bitsPerSecond)
{
    const QString text = formatDataSize(bitsPerSecond, /*precision=*/0);

    return text + QObject::tr("/s");
}
