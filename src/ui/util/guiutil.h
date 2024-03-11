#ifndef GUIUTIL_H
#define GUIUTIL_H

#include <QFont>
#include <QIcon>
#include <QVariant>

class GuiUtil
{
public:
    static void setClipboardData(const QVariant &data);

    static QIcon overlayIcon(const QString &basePath, const QString &overlayPath,
            Qt::Alignment alignment = Qt::AlignRight | Qt::AlignBottom);

    static QFont fontBold();
};

#endif // GUIUTIL_H
