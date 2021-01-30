#include "guiutil.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>

#include "iconcache.h"

void GuiUtil::setClipboardData(const QVariant &data)
{
    QClipboard *clipboard = QGuiApplication::clipboard();

    switch (data.userType()) {
    case QMetaType::QPixmap:
        clipboard->setPixmap(data.value<QPixmap>());
        break;
    case QMetaType::QImage:
        clipboard->setImage(data.value<QImage>());
        break;
    default:
        clipboard->setText(data.toString());
    }
}

QIcon GuiUtil::overlayIcon(
        const QString &basePath, const QString &overlayPath, Qt::Alignment alignment)
{
    const auto key = QString("%1|%2|%3").arg(basePath, overlayPath, QString::number(alignment, 16));

    QPixmap pixmap;
    if (IconCache::find(key, &pixmap))
        return pixmap;

    constexpr int baseWidth = 32;
    constexpr int overlayWidth = 16;
    constexpr int deltaWidth = baseWidth - overlayWidth;

    pixmap = IconCache::file(basePath);
    if (pixmap.width() > baseWidth) {
        pixmap = pixmap.scaled(baseWidth, baseWidth);
    }

    const int dx = (alignment & Qt::AlignRight) ? deltaWidth : 0;
    const int dy = (alignment & Qt::AlignBottom) ? deltaWidth : 0;

    const QRect rect = QRect(dx, dy, overlayWidth, overlayWidth);

    // Paint the overlay
    {
        const QPixmap overlay = IconCache::file(overlayPath);
        QPainter p(&pixmap);
        p.drawPixmap(rect, overlay);
    }

    IconCache::insert(key, pixmap);

    return pixmap;
}
