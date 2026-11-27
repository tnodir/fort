#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QFont>
#include <QIcon>
#include <QVariant>

QT_FORWARD_DECLARE_CLASS(QColor)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QPalette)

class GuiUtil
{
public:
    static void setClipboardData(const QVariant &data);
    static QString getClipboardText();

    static QIcon overlayIcon(const QString &basePath, const QString &overlayPath,
            Qt::Alignment alignment = Qt::AlignRight | Qt::AlignBottom);

    /* The base icon inside the progress arc, clockwise from 12 o'clock */
    static QIcon progressIcon(const QString &basePath, int percent, const QColor &color);

    static QFont fontBold();
    static QFont fontItalic();
    static QFont fontStrikeOut();
    static bool isSameFont(const QFont &f1, const QFont &f2);

    static QPoint globalPos(const QMouseEvent *event);

    /* For the Fusion style on a Qt, which can't set the color scheme */
    static QPalette darkPalette();
};

#endif // GUIUTIL_H
