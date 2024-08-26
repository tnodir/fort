#ifndef FORMATUTIL_H
#define FORMATUTIL_H

#include <QLocale>
#include <QObject>

class FormatUtil : public QObject
{
    Q_OBJECT

public:
    enum SizeFormat : quint8 {
        SizeBase1000 = 0x01, // use factors of 1000
        SizeBase1024 = 0x02, // use SI quantifiers

        SizeFormatIec = 0x04, // B, KiB, MiB, GiB, ...
        SizeFormatSi = 0x08, // B, KB, MB, GB, ...

        SizeBits = 0x10, // b, Kb, Mb, Gb, ...

        SizeIecFormat = (SizeBase1000 | SizeFormatIec), // base 1000, KiB, MiB, GiB, ...
        SizeTraditionalFormat = (SizeBase1024 | SizeFormatSi), // base 1024, KB, MB, GB, ...
        SizeSIFormat = (SizeBase1000 | SizeFormatSi), // base 1000, KB, MB, GB, ...

        SpeedTraditionalFormat = (SizeTraditionalFormat | SizeBits), // Kb/s, Mb/s, ...
        SpeedIecFormat = (SizeIecFormat | SizeBits), // Kib/s, Mib/s, ...
    };

    static int getPower(qint64 value, SizeFormat format = SizeTraditionalFormat);

    static QString formatSize(
            qint64 value, int power, int precision = 2, SizeFormat format = SizeTraditionalFormat);

    static QString formatPowerUnit(int power, SizeFormat format = SizeTraditionalFormat);

    static QString formatDataSize(
            qint64 bytes, int precision = 2, SizeFormat format = SizeTraditionalFormat);

    static QString formatSpeed(qint64 bitsPerSecond, SizeFormat format = SpeedTraditionalFormat);

    static QStringList graphUnitNames();
    static FormatUtil::SizeFormat graphUnitFormat(int index);
};

#endif // FORMATUTIL_H
