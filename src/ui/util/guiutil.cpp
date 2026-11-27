#include "guiutil.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>

#include "iconcache.h"

namespace {

struct PaletteColor
{
    QPalette::ColorRole role;
    QRgb color;
};

// All the roles: a missing one is of the system's light theme
inline constexpr PaletteColor darkPaletteColors[] = {
    { QPalette::Window, qRgb(53, 53, 53) },
    { QPalette::WindowText, qRgb(224, 224, 224) },
    { QPalette::Base, qRgb(36, 36, 36) },
    { QPalette::AlternateBase, qRgb(45, 45, 45) },
    { QPalette::ToolTipBase, qRgb(36, 36, 36) },
    { QPalette::ToolTipText, qRgb(224, 224, 224) },
    { QPalette::PlaceholderText, qRgb(140, 140, 140) },
    { QPalette::Text, qRgb(224, 224, 224) },
    { QPalette::Button, qRgb(53, 53, 53) },
    { QPalette::ButtonText, qRgb(224, 224, 224) },
    { QPalette::BrightText, qRgb(255, 255, 255) },
    { QPalette::Light, qRgb(90, 90, 90) },
    { QPalette::Midlight, qRgb(70, 70, 70) },
    { QPalette::Mid, qRgb(44, 44, 44) },
    { QPalette::Dark, qRgb(30, 30, 30) },
    { QPalette::Shadow, qRgb(0, 0, 0) },
    { QPalette::Highlight, qRgb(42, 130, 218) },
    { QPalette::HighlightedText, qRgb(255, 255, 255) },
    { QPalette::Link, qRgb(86, 163, 255) },
    { QPalette::LinkVisited, qRgb(180, 140, 255) },
};

inline constexpr PaletteColor darkPaletteDisabledColors[] = {
    { QPalette::WindowText, qRgb(127, 127, 127) },
    { QPalette::Base, qRgb(53, 53, 53) },
    { QPalette::Text, qRgb(127, 127, 127) },
    { QPalette::ButtonText, qRgb(127, 127, 127) },
    { QPalette::Highlight, qRgb(80, 80, 80) },
    { QPalette::HighlightedText, qRgb(160, 160, 160) },
};

}

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

QString GuiUtil::getClipboardText()
{
    return QGuiApplication::clipboard()->text();
}

QIcon GuiUtil::overlayIcon(
        const QString &basePath, const QString &overlayPath, Qt::Alignment alignment)
{
    if (overlayPath.isEmpty()) {
        return IconCache::icon(basePath);
    }

    const auto key = QString("%1|%2|%3").arg(basePath, overlayPath, QString::number(alignment, 16));

    QIcon icon;
    if (IconCache::find(key, icon))
        return icon;

    constexpr int xOffset = 2;
    constexpr int yOffset = 3;

    constexpr int baseWidth = 32;
    constexpr int overlayWidth = 23;
    constexpr int deltaWidth = baseWidth - overlayWidth;

    QPixmap pixmap = IconCache::pixmap(basePath, baseWidth);

    const int dx = (alignment & Qt::AlignRight) ? deltaWidth + xOffset : -xOffset;
    const int dy = (alignment & Qt::AlignBottom) ? deltaWidth + yOffset : -yOffset;

    const QRect rect = QRect(dx, dy, overlayWidth, overlayWidth);

    // Paint the overlay
    {
        const QPixmap overlay = IconCache::pixmap(overlayPath, overlayWidth);
        QPainter p(&pixmap);
        p.drawPixmap(rect, overlay);
    }

    icon = QIcon(pixmap);

    IconCache::insert(key, icon);

    return icon;
}

QIcon GuiUtil::progressIcon(const QString &basePath, int percent, const QColor &color)
{
    constexpr int width = 32;
    constexpr int baseWidth = 20;
    constexpr int penWidth = 3;

    QPixmap pixmap(width, width);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);

    // Paint the base
    {
        constexpr int offset = (width - baseWidth) / 2;

        const QPixmap base = IconCache::pixmap(basePath, baseWidth);
        p.drawPixmap(QRect(offset, offset, baseWidth, baseWidth), base);
    }

    // Paint the arc
    {
        constexpr qreal margin = penWidth / 2.0;

        const QRectF rect = QRectF(0, 0, width, width).adjusted(margin, margin, -margin, -margin);

        p.setPen(QPen(color, penWidth, Qt::SolidLine, Qt::FlatCap));
        p.drawArc(rect, 90 * 16, -percent * 360 * 16 / 100);
    }

    p.end();

    return QIcon(pixmap);
}

QFont GuiUtil::fontBold()
{
    QFont font;
    font.setBold(true);
    return font;
}

QFont GuiUtil::fontItalic()
{
    QFont font;
    font.setItalic(true);
    return font;
}

QFont GuiUtil::fontStrikeOut()
{
    QFont font;
    font.setStrikeOut(true);
    return font;
}

bool GuiUtil::isSameFont(const QFont &f1, const QFont &f2)
{
    return f1.family() == f2.family() && f1.pointSize() == f2.pointSize();
}

QPoint GuiUtil::globalPos(const QMouseEvent *event)
{
    return event->globalPosition().toPoint();
}

QPalette GuiUtil::darkPalette()
{
    QPalette palette;

    for (const auto &c : darkPaletteColors) {
        palette.setColor(c.role, QColor(c.color));
    }

    for (const auto &c : darkPaletteDisabledColors) {
        palette.setColor(QPalette::Disabled, c.role, QColor(c.color));
    }

    return palette;
}
