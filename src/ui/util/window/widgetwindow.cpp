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

void WidgetWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *e = static_cast<QWindowStateChangeEvent *>(event);

        if (e->oldState() != this->windowState()) {
            emit visibilityChanged();
        }
    }
}
