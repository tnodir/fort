#include "guiutil.h"

#include <QClipboard>
#include <QColorDialog>
#include <QGuiApplication>
#include <QPixmap>
#include <QImage>

GuiUtil::GuiUtil(QObject *parent) :
    QObject(parent)
{
}

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

QColor GuiUtil::getColor(const QColor &initial)
{
    return QColorDialog::getColor(initial);
}

bool GuiUtil::isValidColor(const QColor &color)
{
    return color.isValid();
}
