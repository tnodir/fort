#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QFont>
#include <QIcon>
#include <QVariant>

QT_FORWARD_DECLARE_CLASS(QColor)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)

class GuiUtil
{
public:
    static void setClipboardData(const QVariant &data);

    static QIcon overlayIcon(const QString &basePath, const QString &overlayPath,
            Qt::Alignment alignment = Qt::AlignRight | Qt::AlignBottom);

    /* The base icon inside the progress arc, clockwise from 12 o'clock */
    static QIcon progressIcon(const QString &basePath, int percent, const QColor &color);

    static QFont fontBold();
    static bool isSameFont(const QFont &f1, const QFont &f2);

    static QPoint globalPos(const QMouseEvent *event);
};

#endif // GUIUTIL_H
