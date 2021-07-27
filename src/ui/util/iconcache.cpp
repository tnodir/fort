#include "iconcache.h"

#include <QDebug>
#include <QIcon>
#include <QPixmapCache>
#include <QThread>

namespace {

void checkThread()
{
#ifndef QT_NO_DEBUG
    static const Qt::HANDLE g_mainThreadId = QThread::currentThreadId();

    const Qt::HANDLE threadId = QThread::currentThreadId();
    if (g_mainThreadId != threadId) {
        qWarning() << "QPixmap used by non-main thread:" << threadId << "; main:" << g_mainThreadId;
    }
#endif
}

}

bool IconCache::find(const QString &key, QPixmap *pixmap)
{
    checkThread();

    return QPixmapCache::find(key, pixmap);
}

bool IconCache::insert(const QString &key, const QPixmap &pixmap)
{
    checkThread();

    return QPixmapCache::insert(key, pixmap);
}

void IconCache::remove(const QString &key)
{
    checkThread();

    return QPixmapCache::remove(key);
}

QPixmap IconCache::file(const QString &filePath)
{
    checkThread();

    QPixmap pixmap;
    if (!find(filePath, &pixmap)) {
        pixmap.load(filePath);
        insert(filePath, pixmap);
    }
    return pixmap;
}

QIcon IconCache::icon(const QString &filePath)
{
    checkThread();

    return filePath.isEmpty() ? QIcon() : QIcon(file(filePath));
}
