#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QFont>
#include <QIcon>
#include <QVariant>

QT_FORWARD_DECLARE_CLASS(QMouseEvent)

class GuiUtil
{
public:
    static void setClipboardData(const QVariant &data);

    static QIcon overlayIcon(const QString &basePath, const QString &overlayPath,
            Qt::Alignment alignment = Qt::AlignRight | Qt::AlignBottom);

    static QFont fontBold();

    static QPoint globalPos(const QMouseEvent *event);
};

#endif // GUIUTIL_H
