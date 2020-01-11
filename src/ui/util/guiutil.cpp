#include "guiutil.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPixmap>

void GuiUtil::setClipboardData(const QVariant &data)
{
    QClipboard *clipboard = QGuiApplication::clipboard();

    switch (data.type()) {
    case QVariant::Pixmap:
        clipboard->setPixmap(data.value<QPixmap>());
        break;
    case QVariant::Image:
        clipboard->setImage(data.value<QImage>());
        break;
    default:
        clipboard->setText(data.toString());
    }
}

QIcon GuiUtil::overlayIcon(const QString &basePath,
                           const QString &overlayPath,
                           Qt::Alignment alignment)
{
    constexpr int baseWidth = 32;
    constexpr int overlayWidth = 16;
    constexpr int deltaWidth = baseWidth - overlayWidth;

    QPixmap base(basePath);
    if (base.width() > baseWidth) {
        base = base.scaled(baseWidth, baseWidth);
    }

    QPixmap overlay(overlayPath);

    const int dx = (alignment & Qt::AlignRight) ? deltaWidth : 0;
    const int dy = (alignment & Qt::AlignBottom) ? deltaWidth : 0;

    const QRect rect = QRect(dx, dy, overlayWidth, overlayWidth);

    // Paint the overlay
    {
        QPainter painter(&base);
        painter.drawPixmap(rect, overlay);
    }

    return base;
}
