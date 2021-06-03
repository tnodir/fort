#include "windowmanagerfake.h"

#include <QDebug>

WindowManagerFake::WindowManagerFake(QObject *parent) : WindowManager(parent) { }

void WindowManagerFake::showErrorBox(const QString &text, const QString &title)
{
    qWarning() << title << ":" << text;
}

void WindowManagerFake::showInfoBox(const QString &text, const QString &title)
{
    qInfo() << title << ":" << text;
}
