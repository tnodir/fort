#include "widgetwindow.h"

#include <QWindowStateChangeEvent>

WidgetWindow::WidgetWindow(QWidget *parent) :
    QWidget(parent)
{
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

    emit visibilityChanged();
}

void WidgetWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);

    emit visibilityChanged();
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
    case Qt::Key_Escape:  // Esc
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
    case QEvent::ActivationChange:
        emit activationChanged();
        break;
    case QEvent::WindowStateChange: {
        auto e = static_cast<QWindowStateChangeEvent *>(event);

        if (e->oldState() != this->windowState()) {
            emit visibilityChanged();
        }
        break;
    }
    default:
        break;
    }
}
