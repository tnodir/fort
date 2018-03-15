#include "windowstatewatcher.h"

#include <QWindow>

WindowStateWatcher::WindowStateWatcher(QObject *parent) :
    BaseWindowStateWatcher(parent)
{
}

void WindowStateWatcher::install(QWindow *window)
{
    connect(window, &QWindow::xChanged, this, &WindowStateWatcher::onRectChanged);
    connect(window, &QWindow::yChanged, this, &WindowStateWatcher::onRectChanged);
    connect(window, &QWindow::widthChanged, this, &WindowStateWatcher::onRectChanged);
    connect(window, &QWindow::heightChanged, this, &WindowStateWatcher::onRectChanged);
    connect(window, &QWindow::visibilityChanged, this, &WindowStateWatcher::onVisibilityChanged);
}

void WindowStateWatcher::onRectChanged()
{
    QWindow *window = qobject_cast<QWindow *>(sender());

    handleRectChange(window->geometry(), window->visibility());
}

void WindowStateWatcher::onVisibilityChanged()
{
    QWindow *window = qobject_cast<QWindow *>(sender());

    handleVisibilityChange(window->visibility());
}

void WindowStateWatcher::restore(QWindow *window, const QSize &defaultSize,
                                 const QRect &rect, bool maximized)
{
    if (rect.isNull()) {
        window->resize(defaultSize);
        return;
    }

    this->reset(rect, maximized);

    window->setGeometry(rect);

    if (maximized) {
        window->setVisibility(QWindow::Maximized);
    }
}
