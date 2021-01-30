#include "iconcache.h"

#include <QIcon>
#include <QPixmapCache>

bool IconCache::find(const QString &key, QPixmap *pixmap)
{
    return QPixmapCache::find(key, pixmap);
}

bool IconCache::insert(const QString &key, const QPixmap &pixmap)
{
    return QPixmapCache::insert(key, pixmap);
}

void IconCache::remove(const QString &key)
{
    return QPixmapCache::remove(key);
}

QPixmap IconCache::file(const QString &filePath)
{
    QPixmap pixmap;
    if (!find(filePath, &pixmap)) {
        pixmap.load(filePath);
        insert(filePath, pixmap);
    }
    return pixmap;
}

QIcon IconCache::icon(const QString &filePath)
{
    return QIcon(file(filePath));
}
