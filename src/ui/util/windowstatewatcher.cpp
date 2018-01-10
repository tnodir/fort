#include "windowstatewatcher.h"

#include <QWindow>

WindowStateWatcher::WindowStateWatcher(QObject *parent) :
    QObject(parent),
    m_maximized(false)
{
}

void WindowStateWatcher::setup(QWindow *window)
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

    if (window->visibility() != QWindow::Windowed)
        return;

    const QRect rect = window->geometry();

    if (rect != m_rect) {
        m_rectPrev = m_rect;
        m_rect = rect;
    }
}

void WindowStateWatcher::onVisibilityChanged()
{
    QWindow *window = qobject_cast<QWindow *>(sender());

    switch (window->visibility()) {
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
