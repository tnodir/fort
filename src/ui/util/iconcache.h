#ifndef ICONCACHE_H
#define ICONCACHE_H

#include <QIcon>
#include <QObject>

class IconCache
{
public:
    static QIcon *findObject(const QString &key);
    static bool insertObject(const QString &key, QIcon *iconObj);

    static bool find(const QString &key, QIcon &icon);
    static bool insert(const QString &key, const QIcon &icon);
    static void remove(const QString &key);

    static QIcon *iconObject(const QString &filePath);
    static QIcon icon(const QString &filePath);

    static QPixmap pixmap(const QString &filePath, const QSize &size);
    static QPixmap pixmap(const QString &filePath, int extent = 32);
};

#endif // ICONCACHE_H
