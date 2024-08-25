#ifndef FORMATUTIL_H
#define FORMATUTIL_H

#include <QLocale>
#include <QObject>

class FormatUtil : public QObject
{
    Q_OBJECT

public:
    enum SizeFormat {
        SizeBase1000 = 0x01, // use factors of 1000
        SizeBase1024 = 0x02, // use SI quantifiers

        SizeFormatIec = 0x04, // B, KiB, MiB, GiB, ...
        SizeFormatSi = 0x08, // B, KB, MB, GB, ...

        SizeBits = 0x10, // b, Kb, Mb, Gb, ...

        SizeIecFormat = (SizeBase1024 | SizeFormatIec), // base 1024, KiB, MiB, GiB, ...
        SizeTraditionalFormat = (SizeBase1024 | SizeFormatSi), // base 1024, kB, MB, GB, ...
        SizeSIFormat = (SizeBase1000 | SizeFormatSi) // base 1000, kB, MB, GB, ...
    };

    static int getPower(qint64 value, SizeFormat format = SizeTraditionalFormat);

    static QString formatSize(
            qint64 value, int power, int precision = 2, SizeFormat format = SizeTraditionalFormat);

    static QString formatPowerUnit(int power, SizeFormat format = SizeTraditionalFormat);

    static QString formatDataSize(
            qint64 bytes, int precision = 2, SizeFormat format = SizeTraditionalFormat);

    static QString formatSpeed(quint32 bitsPerSecond);
};

#endif // FORMATUTIL_H
