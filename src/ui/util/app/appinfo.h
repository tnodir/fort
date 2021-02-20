#ifndef APPINFO_H
#define APPINFO_H

#include <QDateTime>
#include <QObject>

class AppInfo
{
    Q_GADGET
    Q_PROPERTY(QString fileDescription MEMBER fileDescription CONSTANT)
    Q_PROPERTY(QString companyName MEMBER companyName CONSTANT)
    Q_PROPERTY(QString productName MEMBER productName CONSTANT)
    Q_PROPERTY(QString productVersion MEMBER productVersion CONSTANT)

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

Q_DECLARE_METATYPE(AppInfo)

#endif // APPINFO_H
