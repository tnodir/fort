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
    auto window = qobject_cast<WidgetWindow *>(sender());

    handleWindowPositionChange(window);
}

void WidgetWindowStateWatcher::onSizeChanged()
{
    auto window = qobject_cast<WidgetWindow *>(sender());

    handleWindowSizeChange(window);
}

void WidgetWindowStateWatcher::onVisibilityChanged()
{
    auto window = qobject_cast<WidgetWindow *>(sender());

    handleWindowVisibilityChange(window);
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
