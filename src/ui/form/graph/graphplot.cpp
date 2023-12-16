#include "graphplot.h"

GraphPlot::GraphPlot(QWidget *parent) : QCustomPlot(parent) { }

void GraphPlot::cancelMousePressAndDragging()
{
    m_mousePressed = false;
    m_mouseDragging = false;
}

void GraphPlot::resizeEvent(QResizeEvent *event)
{
    QCustomPlot::resizeEvent(event);

    emit resized(event);
}

void GraphPlot::mousePressEvent(QMouseEvent *event)
{
    QCustomPlot::mousePressEvent(event);

    m_mousePressed = true;
}

void GraphPlot::mouseMoveEvent(QMouseEvent *event)
{
    QCustomPlot::mouseMoveEvent(event);

    if (!mMouseHasMoved || !m_mousePressed)
        return;

    if (!m_mouseDragging) {
        m_mouseDragging = true;
        emit mouseDragBegin(event);
    }

    emit mouseDragMove(event);
}

void GraphPlot::mouseReleaseEvent(QMouseEvent *event)
{
    QCustomPlot::mouseReleaseEvent(event);

    if (!m_mousePressed)
        return;

    m_mousePressed = false;

    if (m_mouseDragging) {
        m_mouseDragging = false;
        emit mouseDragEnd(event);
    } else if (event->button() == Qt::RightButton) {
        emit mouseRightClick(event);
    }
}
