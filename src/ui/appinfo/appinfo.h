#ifndef APPINFO_H
#define APPINFO_H

#include <QDateTime>
#include <QObject>

class AppInfo
{
public:
    QString filePath(const QString &appPath) const;
    bool checkFileModified(const QString &appPath);

    bool isValid() const { return iconId != 0; }

public:
    bool fileExists = true; // transient

    qint64 iconId = 0;

    QDateTime fileModTime;

    QString altPath;
    QString fileDescription;
    QString companyName;
    QString productName;
    QString productVersion;
};

Q_DECLARE_METATYPE(AppInfo)

#endif // APPINFO_H
