#include "basewindowstatewatcher.h"

BaseWindowStateWatcher::BaseWindowStateWatcher(QObject *parent) : QObject(parent) { }

QRect BaseWindowStateWatcher::geometry() const
{
    return { m_pos, m_size };
}

void BaseWindowStateWatcher::setGeometry(const QRect &rect)
{
    m_pos = m_posPrev = rect.topLeft();
    m_size = m_sizePrev = rect.size();
}

void BaseWindowStateWatcher::reset(const QRect &rect, bool maximized)
{
    setGeometry(rect);
    setMaximized(maximized);
}

void BaseWindowStateWatcher::uninstall(QObject *window)
{
    disconnect(window);
}

void BaseWindowStateWatcher::handlePositionChange(const QPoint &pos, QWindow::Visibility visibility)
{
    if (visibility != QWindow::Windowed)
        return;

    if (pos != m_pos) {
        m_posPrev = m_pos;
        m_pos = pos;
    }
}

void BaseWindowStateWatcher::handleSizeChange(const QSize &size, QWindow::Visibility visibility)
{
    if (visibility != QWindow::Windowed)
        return;

    if (size != m_size) {
        m_sizePrev = m_size;
        m_size = size;
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
        m_pos = m_posPrev;
        m_size = m_sizePrev;
        break;
    default:
        break;
    }

    m_visible = (visibility != QWindow::Hidden);
}
