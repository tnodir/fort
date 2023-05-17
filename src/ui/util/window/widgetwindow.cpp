#include "widgetwindow.h"

#include <QWindowStateChangeEvent>

WidgetWindow::WidgetWindow(QWidget *parent) : QWidget(parent) { }

void WidgetWindow::showWidget(QWidget *w)
{
    if (w->isMinimized()) {
        w->setWindowState(w->windowState() ^ Qt::WindowMinimized);
    }
    w->show();
    w->raise();
    w->activateWindow();
}

void WidgetWindow::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);

    emit positionChanged();
}

void WidgetWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    emit sizeChanged();
}

void WidgetWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    emit visibilityChanged(/*isVisible=*/true);
}

void WidgetWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);

    emit visibilityChanged(/*isVisible=*/false);
}

void WidgetWindow::closeEvent(QCloseEvent *event)
{
    emit aboutToClose();

    QWidget::closeEvent(event);
}

void WidgetWindow::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);

    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_Escape: // Esc
        if (event->modifiers() == Qt::NoModifier) {
            close();
        }
        break;
    }
}

void WidgetWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    switch (event->type()) {
    case QEvent::WindowStateChange: {
        auto e = static_cast<QWindowStateChangeEvent *>(event);

        const Qt::WindowStates newState = windowState();
        if (newState != e->oldState()) {
            emit visibilityChanged(/*isVisible=*/(newState != Qt::WindowNoState));
        }
    } break;
    default:
        break;
    }
}

bool WidgetWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate: {
        emit activationChanged(/*isActive=*/true);
        emit activated();
    } break;
    case QEvent::WindowDeactivate: {
        emit activationChanged(/*isActive=*/false);
        emit deactivated();
    } break;
    default:
        break;
    }

    return QWidget::event(event);
}
