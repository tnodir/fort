#include "osutil.h"

#include <QApplication>
#include <QClipboard>
#include <QPixmap>
#include <QImage>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

bool OsUtil::createGlobalMutex(const char *name)
{
    return !CreateMutexA(NULL, FALSE, name);
}
