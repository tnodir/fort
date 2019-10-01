#ifndef APPINFO_H
#define APPINFO_H

#include <QDateTime>
#include <QObject>

class AppInfo
{
    Q_GADGET
    Q_PROPERTY(QString iconPath READ iconPath CONSTANT)
    Q_PROPERTY(QString fileDescription MEMBER fileDescription CONSTANT)
    Q_PROPERTY(QString companyName MEMBER companyName CONSTANT)
    Q_PROPERTY(QString productName MEMBER productName CONSTANT)
    Q_PROPERTY(QString productVersion MEMBER productVersion CONSTANT)

public:
    QString iconPath() const;

    bool isFileModified(const QString &appPath) const;

public:
    qint64 iconId = 0;

    QString fileDescription;
    QString companyName;
    QString productName;
    QString productVersion;

    QDateTime fileModTime;
};

Q_DECLARE_METATYPE(AppInfo)

#endif // APPINFO_H
