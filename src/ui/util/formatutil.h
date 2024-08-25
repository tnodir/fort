#ifndef FORMATUTIL_H
#define FORMATUTIL_H

#include <QLocale>
#include <QObject>

class FormatUtil : public QObject
{
    Q_OBJECT

public:
    static QString formatDataSize(qint64 bytes, int precision = 2,
            QLocale::DataSizeFormats format = QLocale::DataSizeTraditionalFormat);

    static QString formatSpeed(quint32 bitsPerSecond);
};

#endif // FORMATUTIL_H
