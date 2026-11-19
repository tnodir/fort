#include "widgetwindowstatewatcher.h"

#include "widgetwindow.h"

WidgetWindowStateWatcher::WidgetWindowStateWatcher(QObject *parent) : BaseWindowStateWatcher(parent)
{
}

void WidgetWindowStateWatcher::install(WidgetWindow *window)
{
    connect(window, &WidgetWindow::positionChanged, this,
            &WidgetWindowStateWatcher::onPositionChanged);
    connect(window, &WidgetWindow::sizeChanged, this, &WidgetWindowStateWatcher::onSizeChanged);
    connect(window, &WidgetWindow::visibilityChanged, this,
            &WidgetWindowStateWatcher::onVisibilityChanged);
}

void WidgetWindowStateWatcher::onPositionChanged()
{
    if (auto window = senderWindow()) {
        handleWindowPositionChange(window);
    }
}

void WidgetWindowStateWatcher::onSizeChanged()
{
    if (auto window = senderWindow()) {
        handleWindowSizeChange(window);
    }
}

void WidgetWindowStateWatcher::onVisibilityChanged()
{
    if (auto window = senderWindow()) {
        handleWindowVisibilityChange(window);
    }
}

// A docked window's geometry isn't a window's one
WidgetWindow *WidgetWindowStateWatcher::senderWindow() const
{
    auto window = qobject_cast<WidgetWindow *>(sender());

    return window->isWindow() ? window : nullptr;
}

void WidgetWindowStateWatcher::handleWindowPositionChange(WidgetWindow *window)
{
    handlePositionChange(window->geometry().topLeft(), getVisibility(window));
}

void WidgetWindowStateWatcher::handleWindowSizeChange(WidgetWindow *window)
{
    handleSizeChange(window->size(), getVisibility(window));
}

void WidgetWindowStateWatcher::handleWindowVisibilityChange(WidgetWindow *window)
{
    if (!visible()) {
        handleWindowPositionChange(window);
        handleWindowSizeChange(window);
    }
    handleVisibilityChange(getVisibility(window));
}

void WidgetWindowStateWatcher::restore(
        WidgetWindow *window, const QSize &defaultSize, const QRect &rect, bool maximized)
{
    if (rect.isNull()) {
        window->resize(defaultSize);
        return;
    }

    this->reset(rect, maximized);

    // Before setGeometry(): a visible maximized window is moved by it, keeping its old normal
    // geometry
    window->setWindowState(window->windowState() & ~Qt::WindowMaximized);

    window->setGeometry(rect);

    if (maximized) {
        window->setWindowState(Qt::WindowMaximized);
    }
}

QWindow::Visibility WidgetWindowStateWatcher::getVisibility(WidgetWindow *window)
{
    if (!window->isVisible())
        return QWindow::Hidden;

    const int state(int(window->windowState())
            & int(Qt::WindowMinimized | Qt::WindowMaximized | Qt::WindowFullScreen));

    switch (state) {
    case Qt::WindowMinimized:
        return QWindow::Minimized;
    case Qt::WindowMaximized:
        return QWindow::Maximized;
    case Qt::WindowFullScreen:
        return QWindow::FullScreen;
    default:
        return QWindow::Windowed;
    }
}
