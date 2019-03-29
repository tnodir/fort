#ifndef APPUTIL_H
#define APPUTIL_H

#include <QObject>
#include <QPixmap>

class AppUtil : public QObject
{
    Q_OBJECT

public:
    explicit AppUtil(QObject *parent = nullptr);

    Q_INVOKABLE static QPixmap getIcon(const QString &appPath);
};

#endif // APPUTIL_H
