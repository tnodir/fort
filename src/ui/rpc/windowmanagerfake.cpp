#include "windowmanagerfake.h"

#include <QLoggingCategory>

#include <fortsettings.h>
#include <util/ioc/ioccontainer.h>

namespace {
const QLoggingCategory LC("rpc.window");
}

WindowManagerFake::WindowManagerFake(QObject *parent) : WindowManager(parent) { }

bool WindowManagerFake::exposeHomeWindow()
{
    return false;
}

bool WindowManagerFake::showProgramEditForm(const QString & /*appPath*/)
{
    return false;
}

bool WindowManagerFake::checkPassword(bool /*temporary*/)
{
    return !IoC<FortSettings>()->hasPassword();
}

void WindowManagerFake::showErrorBox(
        const QString &text, const QString &title, QWidget * /*parent*/)
{
    qCWarning(LC) << title << ":" << text;
}

void WindowManagerFake::showInfoBox(const QString &text, const QString &title, QWidget * /*parent*/)
{
    qCInfo(LC) << title << ":" << text;
}
