#include "iconcache.h"

#include <QCache>
#include <QDir>
#include <QIcon>
#include <QLoggingCategory>

#include "fileutil.h"

namespace {

const QLoggingCategory LC("util.iconCache");

constexpr int CacheLimitCount = 200; // icons max count

static struct IconCachePrivate
{
    IconCachePrivate() : cache(CacheLimitCount) { }

    bool dirChecked : 1 = false;
    bool dirExists : 1 = false;

    QCache<QString, QIcon> cache;
} g_iconCache;

QString adjustFilePath(const QString &filePath)
{
    if (!g_iconCache.dirChecked) {
        g_iconCache.dirChecked = true;
        g_iconCache.dirExists = QDir().exists("icons");
    }

    if (!g_iconCache.dirExists)
        return filePath;

    // Try to use "./icons/" folder from current working dir.
    if (filePath.startsWith(':')) {
        QString adjustedFilePath = filePath;
        adjustedFilePath[0] = '.';

        if (FileUtil::fileExists(adjustedFilePath))
            return adjustedFilePath;
    }

    return filePath;
}

}

QIcon *IconCache::findObject(const QString &key)
{
    return g_iconCache.cache.object(key);
}

bool IconCache::insertObject(const QString &key, QIcon *iconObj)
{
    return g_iconCache.cache.insert(key, iconObj);
}

bool IconCache::find(const QString &key, QIcon &icon)
{
    const QIcon *iconObj = findObject(key);

    if (iconObj) {
        icon = *iconObj;
        return true;
    }
    return false;
}

bool IconCache::insert(const QString &key, const QIcon &icon)
{
    QIcon *iconObj = new QIcon(icon);

    return insertObject(key, iconObj);
}

void IconCache::remove(const QString &key)
{
    g_iconCache.cache.remove(key);
}

QIcon *IconCache::iconObject(const QString &filePath)
{
    QIcon *iconObj = findObject(filePath);

    if (!iconObj) {
        iconObj = new QIcon(adjustFilePath(filePath));

        if (!insertObject(filePath, iconObj)) {
            iconObj = nullptr;
        }
    }
    return iconObj;
}

QIcon IconCache::icon(const QString &filePath)
{
    if (filePath.isEmpty())
        return {};

    QIcon *iconObj = iconObject(filePath);
    if (iconObj) {
        return *iconObj;
    }

    return QIcon(adjustFilePath(filePath));
}

QPixmap IconCache::pixmap(const QString &filePath, const QSize &size)
{
    if (filePath.isEmpty())
        return {};

    QIcon *iconObj = iconObject(filePath);
    if (iconObj) {
        return iconObj->pixmap(size);
    }

    return icon(filePath).pixmap(size);
}

QPixmap IconCache::pixmap(const QString &filePath, int extent)
{
    const QSize size(extent, extent);

    return pixmap(filePath, size);
}
