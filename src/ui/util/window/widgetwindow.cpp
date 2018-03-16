#include "widgetwindow.h"

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
