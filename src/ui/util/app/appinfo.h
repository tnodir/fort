#ifndef APPINFO_H
#define APPINFO_H

#include <QPixmap>

class AppInfo
{
    Q_GADGET
    Q_PROPERTY(QString fileDescription MEMBER fileDescription CONSTANT)
    Q_PROPERTY(QString companyName MEMBER companyName CONSTANT)
    Q_PROPERTY(QString productName MEMBER productName CONSTANT)
    Q_PROPERTY(QString productVersion MEMBER productVersion CONSTANT)
    Q_PROPERTY(QPixmap icon MEMBER icon CONSTANT)

public:
    QString fileDescription;
    QString companyName;
    QString productName;
    QString productVersion;

    QPixmap icon;
};

Q_DECLARE_METATYPE(AppInfo)

#endif // APPINFO_H
