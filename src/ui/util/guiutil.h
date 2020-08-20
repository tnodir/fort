#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QIcon>
#include <QVariant>

class GuiUtil
{
public:
    static void setClipboardData(const QVariant &data);

    static QIcon overlayIcon(const QString &basePath, const QString &overlayPath,
            Qt::Alignment alignment = Qt::AlignRight | Qt::AlignBottom);
};

#endif // GUIUTIL_H
