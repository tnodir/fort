#include "osutil.h"

#include <QApplication>
#include <QClipboard>
#include <QPixmap>
#include <QImage>

#include "processinfo.h"

OsUtil::OsUtil(QObject *parent) :
    QObject(parent)
{
}

void OsUtil::setClipboardData(const QVariant &data)
{
    QClipboard *clipboard = QApplication::clipboard();

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

QString OsUtil::pidToDosPath(quint32 pid)
{
    const ProcessInfo pi(pid);
    return pi.dosPath();
}
