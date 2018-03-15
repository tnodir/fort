#include "basewindowstatewatcher.h"

BaseWindowStateWatcher::BaseWindowStateWatcher(QObject *parent) :
    QObject(parent),
    m_maximized(false)
{
}

void BaseWindowStateWatcher::uninstall(QObject *window)
{
    disconnect(window);
}

void BaseWindowStateWatcher::handleRectChange(const QRect &rect,
                                              QWindow::Visibility visibility)
{
    if (visibility != QWindow::Windowed)
        return;

    if (rect != m_rect) {
        m_rectPrev = m_rect;
        m_rect = rect;
    }
}

void BaseWindowStateWatcher::handleVisibilityChange(QWindow::Visibility visibility)
{
    switch (visibility) {
    case QWindow::Windowed:
        m_maximized = false;
        break;
    case QWindow::Maximized:
        m_maximized = true;
        m_rect = m_rectPrev;
        break;
    default: break;
    }
}
