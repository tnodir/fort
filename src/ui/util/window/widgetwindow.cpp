#include "widgetwindow.h"

#include <QWindowStateChangeEvent>

namespace {

void bringWindowToTop(QWidget *w)
{
    const auto flags = w->windowFlags();
    if ((flags & Qt::WindowStaysOnTopHint) != 0)
        return;

    w->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    w->show();

    w->setWindowFlags(flags);
    w->show();
}

}

WidgetWindow::WidgetWindow(QWidget *parent) : QWidget(parent) { }

void WidgetWindow::showWindow(bool activate)
{
    if (isHidden()) {
        emit aboutToShow();
    }

    showWidget(this, activate);
}

void WidgetWindow::showWidget(QWidget *w, bool activate)
{
    if (w->isMinimized()) {
        w->setWindowState(w->windowState() ^ Qt::WindowMinimized);
    }

    w->show();
    w->raise();

    if (activate) {
        bringWindowToTop(w);

        w->activateWindow();
    }
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
    event->accept();

    emit aboutToClose(event);
}

void WidgetWindow::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);

    switch (event->key()) {
    case Qt::Key_Escape:
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
    const QEvent::Type type = event->type();
    const bool res = QWidget::event(event);

    switch (type) {
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate: {
        const bool isActive = (type == QEvent::WindowActivate);
        emit activationChanged(isActive);
    } break;
    default:
        break;
    }

    return res;
}
