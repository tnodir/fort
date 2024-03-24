#include "guiutil.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QMouseEvent>
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

    constexpr int xOffset = 2;
    constexpr int yOffset = 3;

    constexpr int baseWidth = 32;
    constexpr int overlayWidth = 23;
    constexpr int deltaWidth = baseWidth - overlayWidth;

    pixmap = IconCache::file(basePath);
    if (pixmap.width() > baseWidth) {
        pixmap = pixmap.scaled(baseWidth, baseWidth);
    }

    const int dx = (alignment & Qt::AlignRight) ? deltaWidth + xOffset : -xOffset;
    const int dy = (alignment & Qt::AlignBottom) ? deltaWidth + yOffset : -yOffset;

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

QFont GuiUtil::fontBold()
{
    QFont font;
    font.setBold(true);
    return font;
}

QPoint GuiUtil::globalPos(const QMouseEvent *event)
{
    return event->globalPosition().toPoint();
}
