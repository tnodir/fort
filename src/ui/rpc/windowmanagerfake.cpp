#include "windowmanagerfake.h"

#include <QLoggingCategory>

#include <fortglobal.h>
#include <fortsettings.h>

using namespace Fort;

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

bool WindowManagerFake::checkPassword(WindowCode /*code*/)
{
    return !settings()->hasPassword();
}

void WindowManagerFake::showErrorBox(
        const QString &text, const QString &title, QWidget * /*parent*/)
{
    qCWarning(LC) << title << ":" << text;
}

void WindowManagerFake::showInfoBox(const QString &text, const QString &title, QWidget * /*parent*/)
{
    qCDebug(LC) << title << ":" << text;
}
