#ifndef ZONE_H
#define ZONE_H

#include <QDateTime>
#include <QObject>

class Zone
{
public:
    bool isNameEqual(const Zone &o) const;
    bool isOptionsEqual(const Zone &o) const;

public:
    bool enabled = true;
    bool customUrl = false;

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
