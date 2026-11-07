#ifndef ZONE_H
#define ZONE_H

#include <QDateTime>
#include <QObject>

class Zone
{
public:
    static constexpr quint32 zoneBit(int zoneId) { return quint32(1) << (zoneId - 1); }

    bool isNameEqual(const Zone &o) const;
    bool isOptionsEqual(const Zone &o) const;

public:
    bool enabled : 1 = true;
    bool customUrl : 1 = false;

    int zoneId = 0;

    int addressCount = 0;

    QString zoneName;
    QString sourceCode;

    QString url;
    QString formData;

    QString textInline;

    QString textChecksum;
    QString binChecksum;

    QDateTime sourceModTime;
    QDateTime lastRun;
    QDateTime lastSuccess;
};

#endif // ZONE_H
