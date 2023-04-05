#include "windowmanagerfake.h"

#include <QLoggingCategory>

namespace {
const QLoggingCategory LC("rpc.window");
}

WindowManagerFake::WindowManagerFake(QObject *parent) : WindowManager(parent) { }

void WindowManagerFake::showErrorBox(
        const QString &text, const QString &title, QWidget * /*parent*/)
{
    qCWarning(LC) << title << ":" << text;
}

void WindowManagerFake::showInfoBox(const QString &text, const QString &title, QWidget * /*parent*/)
{
    qCInfo(LC) << title << ":" << text;
}
