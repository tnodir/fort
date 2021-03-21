#ifndef APPINFO_H
#define APPINFO_H

#include <QDateTime>
#include <QObject>

class AppInfo
{
public:
    bool isFileModified(const QString &appPath) const;

    bool isValid() const { return iconId != 0; }

public:
    qint64 iconId = 0;

    QDateTime fileModTime;

    QString fileDescription;
    QString companyName;
    QString productName;
    QString productVersion;
};

#endif // APPINFO_H
