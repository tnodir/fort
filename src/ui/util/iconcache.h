#ifndef ICONCACHE_H
#define ICONCACHE_H

#include <QObject>

class IconCache
{
public:
    static bool find(const QString &key, QPixmap *pixmap);
    static bool insert(const QString &key, const QPixmap &pixmap);
    static void remove(const QString &key);

    static QPixmap file(const QString &filePath);
    static QIcon icon(const QString &filePath);
};

#endif // ICONCACHE_H
